#include "config.h"
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/file.h>
#include <sys/mman.h>
#include <csignal>
#include <cstring>
#include <cerrno>
#include <boost/property_tree/ptree.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/lexical_cast.hpp>
#include <curl/curl.h>
#include "apputils.hpp"
#include "cregex.hpp"
#include "cstate.hpp"
#include "parser.hpp"
#include "cassigner.hpp"
#include "preforked.hpp"
#include "database.hpp"
#include "http.hpp"

namespace pt = boost::property_tree;
extern pt::ptree *cpt;                       // property tree: global configuration

/**
 * in configuration file we need to specify all scripts to parse as
 * scriptName = scriptFile:entryPoint
 * scriptName SHOULD be unique but scriptFile MAY not
 * So we may have more that one scriptName with the same scriptFile and different
 * entryPoints
 */
extern scriptMap_t allScripts;              /// all parsed scripts (scriptName -> scriptMap)
extern entryMap_t  scriptEntries;           /// entry points (scriptName -> entryPoint)

// current children count
volatile static int children_count = 0;     // children started
volatile static int children_running = 0;   // children running ( == children_count if
                                            // there where no crashes)

volatile static bool doRestart = true;      // restart children flag

static char tmp_lockfile[] = "/tmp/appslck.XXXXXX";
static char shr_lockfile[] = "/tmp/appshrd.XXXXXX";
static int  tmp_lockfd;
static int  shr_lockfd;
static int  fcgi_socket;

// RAII-style scoped file lock-unlock
class CScopedFLock {
    int m_fd;
public:
    explicit CScopedFLock(int fd) throw(std::runtime_error) : m_fd(fd) {
        if(flock(m_fd, LOCK_EX) < 0) throw std::runtime_error(strerror(errno));
    }
    ~CScopedFLock() {
        flock(m_fd, LOCK_UN);
    }
};
    
// children slots in shared memory
pslot_t* pslots;

static void sigchld_handler(int sig) {
    int rv;
    pid_t pid;
    while ((pid = waitpid(-1, &rv, 0) > 0)) {
       children_running--;
    }
}

static void sighup_handler_parent(int sig) {
    doRestart = false;
}

static void sigterm_handler_parent(int sig) {
    doRestart = false;
    for(int i = 0; i < CHILDREN_HARDLIMIT; i++) {
        if(pslots[i].slotbusy == 1) {
            log_debug("SIGINT: killing %d", pslots[i].childpid);
            kill(pslots[i].childpid, SIGTERM);
        }
    }
}

extern void freeAllScripts();

void child_atexit_handler() {
    freeAllScripts();
    freeRegexCollection();
    delete cpt;
    close(shr_lockfd);
    close(tmp_lockfd);
    disconnectDBs();
}

static void sigterm_handler_child(int sig) {   
    log_debug("%d: %d killed", sig, getpid());
    exit(0);
}

static void setHandler(int sig, void (*sighandler)(int)) {
    struct sigaction act;
    bzero (&act, sizeof(act));
    act.sa_handler = sighandler;
    act.sa_flags = SA_RESTART | SA_NOCLDSTOP;
    if(sigaction(sig, &act, 0) < 0)
        log_error("%s: sigaction(%d) failed: %s", __func__, sig, strerror(errno));
}

extern inline void setSlotState(int num, slotstate_t state) {
    CScopedFLock fl(shr_lockfd);
    pslots[num].childsts = state;
}

static int processRequest(int child_number) {
    int rv = 0;
    FCGX_Request request;
    CAssigner *assigner = 0;
    char *params = new char[PARAMBUF_LENGTH];
    const std::string scriptSelector = cpt->get<std::string>("common.scriptselector", "@0.function");

    atexit(child_atexit_handler);
    setHandler(SIGTERM, sigterm_handler_child);
    setHandler(SIGINT,  sigterm_handler_child);
    signal(SIGCHLD, SIG_DFL);
    signal(SIGHUP, SIG_DFL);

    try {
        assigner = new CAssigner(cpt->get<std::string>("common.memcached", "--SERVER=localhost --TCP-KEEPALIVE"));
    }
    catch(std::runtime_error &e) {
        log_error("%s:%d: failed to connect to memcached: %s", __func__, child_number, e.what());
    }
    catch(...) {
        log_error("%s:%d: FATAL: possible out of memory!", __func__, child_number);
    }

    try {
        connectDBs();
    }
    catch(std::runtime_error &e) {
        log_error("%s:%d: failed to connect to database: %s", __func__, child_number, e.what());
    }
    catch(...) {
        log_error("%s:%d: FATAL: possible out of memory!", __func__, child_number);
    }

    tmp_lockfd = open(tmp_lockfile, O_RDWR, 0640);
    if(tmp_lockfd < 0)
        log_error("%s:%d: open(tmp_lockfd) failed: %s", __func__, child_number, strerror(errno));

    shr_lockfd = open(shr_lockfile, O_RDWR, 0640);
    if(shr_lockfd < 0)
        log_error("%s:%d: open(shr_lockfd) failed: %s", __func__, child_number, strerror(errno));

    {
        CScopedFLock fl(shr_lockfd);
        pslots[child_number].slotbusy = 1;
        pslots[child_number].childsts = ST_BUSY;
        pslots[child_number].childpid = getpid();
    }
    
    rv = FCGX_InitRequest(&request, fcgi_socket, 0);
    if(rv) log_error("%s:%d: FCGX_InitRequest error: %s", __func__, child_number, strerror(rv));

    // initialize libcurl
    curl_global_init(CURL_GLOBAL_ALL);

    log_debug("%s:%d: child started", __func__, child_number);

    setSlotState(child_number, ST_IDLE);

    while(1) {
        {
            CScopedFLock fl(tmp_lockfd);
            rv = FCGX_Accept_r(&request);
            setSlotState(child_number, ST_BUSY);
        }
        
        if(rv) log_error("%s:%d: FCGX_Accept_r error: %s", __func__, child_number, strerror(rv));

        // parse QUERY_STRING
        // 1. Copy QUERY_STRING to the params buffer
        bzero(params, PARAMBUF_LENGTH*sizeof(char));

        const char *request_method  = FCGX_GetParam("REQUEST_METHOD", request.envp);
        const char *server_protocol = FCGX_GetParam("SERVER_PROTOCOL", request.envp);
        const char *query_string;
        const char *content_length  = FCGX_GetParam("CONTENT_LENGTH", request.envp);
        int paramlen = 0;

        if(strncasecmp(request_method, "GET", 3) == 0) {
            query_string = FCGX_GetParam("QUERY_STRING", request.envp);
            if(query_string) paramlen = strlen(query_string);
        }
        else if(strncasecmp(request_method, "POST", 4) == 0) {
            if(content_length && strlen(content_length) > 0)
                paramlen = boost::lexical_cast<int>(content_length);
            if(paramlen) {
                if(paramlen > PARAMBUF_LENGTH-1) paramlen = PARAMBUF_LENGTH-1;
                FCGX_GetStr(params, paramlen, request.in);
                query_string = params;
            }
        }
        if(!paramlen) {
            FCGX_PutS(HTTPstatus(server_protocol, 404, params, PARAMBUF_LENGTH*sizeof(char)), request.out);
            log_warning("No CGI parameters passed: nothing to do!");
            FCGX_Finish_r(&request);
            setSlotState(child_number, ST_IDLE);
            continue;
        }

        { // 2. split params buffer
            std::vector<std::string> splitted;
            std::string paramstr(query_string);
            paramstr = urlDecode(paramstr);
            boost::split(splitted, paramstr, boost::is_any_of("&"));
            for(unsigned int i = 0; i < splitted.size(); i++) {
                std::string::size_type n = splitted[i].find("=");
                if(n != std::string::npos) {
                    std::string lval("@0.");
                    lval += splitted[i].substr(0, n);
                    std::string rval = splitted[i].substr(n+1);
                    assigner->assignLocal(lval, strdup(rval.c_str()));
                }
            }
        }

        // store all CGI environment variable as local variables of state 0
        for(int i = 0; request.envp[i]; i++) {
            strncpy(params, request.envp[i], PARAMBUF_LENGTH*sizeof(char));
            char *rval = strstr(params, "=");
            if(rval) {
                std::string lval = "@0.";
                *rval = '\0';
                lval += params;
                assigner->assignLocal(lval, strdup(rval+1));
            }
        }

        char *scriptName = assigner->getLocalPtr(scriptSelector);
        if(scriptName) {
            log_message("%s requested", scriptName);
            int nextState;
            stateMap_t *script; // current script to execute
            //  e_it - <scriptName, entryPoint> -- entry point of current script
            const auto e_it = scriptEntries.find(scriptName);
            if(e_it != scriptEntries.end()) nextState = e_it->second;
            else {
                log_warning("%s: entry point not found", scriptName);
                FCGX_PutS(HTTPstatus(server_protocol, 404, params, PARAMBUF_LENGTH*sizeof(char)),
                          request.out);
                goto r_finish;
            }
            // s_it - <scriptName, parsed script state map> 
            const auto s_it = allScripts.find(scriptName);
            if(s_it != allScripts.end()) script = s_it->second;
            else {
                log_warning("%s: script not found", scriptName);
                FCGX_PutS(HTTPstatus(server_protocol, 404, params, PARAMBUF_LENGTH*sizeof(char)), request.out);
                goto r_finish;
            }

            FCGX_PutS("Content-type: text/html\r\n\r\n", request.out);
            // here are the _most_valuable_ten_strings_in_the_program_
            do {
                // sit - <stateNum, CState*> -- current state
                const auto sit = script->find(nextState);
                if(sit == script->end()) {
                    log_warning("Script %s: failed to find state %d", scriptName, nextState);
                    FCGX_PutS(HTTPstatus(server_protocol, 500, params, PARAMBUF_LENGTH*sizeof(char)), request.out);
                    goto r_finish;
                }
                nextState = sit->second->execute(&request, assigner);
            } while(nextState != ENDSTATE);
            log_message("%s: finished", scriptName);
        }
        else {
            FCGX_PutS(HTTPstatus(server_protocol, 404, params, PARAMBUF_LENGTH*sizeof(char)), request.out);
        }

r_finish:
        FCGX_Finish_r(&request);
        assigner->resetTable();
        setSlotState(child_number, ST_IDLE);
    }
    curl_global_cleanup();
    return rv;
}

static inline bool procRunning(pid_t pid) {
    char procdir[64];
    snprintf(procdir, sizeof(procdir), "/proc/%d", pid);
    return eaccess(procdir, F_OK) == 0;
}

static inline void forkChild(int num) {
    int rv = fork();
    if(rv == 0) processRequest(num);
    else if(rv > 0) children_running++;
    else log_error("%s: fork failed: %s", __func__, strerror(errno));
}

static void forkChildren() {
    int quantum = CHILDREN_QUANTUM;
    if(!doRestart) return;
    if(children_count != children_running) {
        // we have holes
        for(int i = 0; i < children_count && quantum > 0; i++, quantum--) {
            if(!procRunning(pslots[i].childpid)) {
                {
                    CScopedFLock fl(shr_lockfd);
                    bzero(&pslots[i], sizeof(pslots[i]));
                }
                forkChild(i);
            }
        }
        assert(children_count == children_running);
    }
    while(quantum && children_count < CHILDREN_HARDLIMIT) {
        forkChild(children_count++);
        quantum--;
    }
}

static bool needFork() {
    float running = 0.0;
    float threshold;

    if(children_count == CHILDREN_HARDLIMIT || !doRestart) return false;
    {
        CScopedFLock fl(shr_lockfd);
        for(int i = 0; i < children_count; i++) {
            if(procRunning(pslots[i].childpid) &&
               pslots[i].slotbusy == 1 &&
               pslots[i].childsts == ST_BUSY) running += 1.0;
        }
    }
    threshold = running / (float)children_count;
    log_debug("%s: busy %.0f children out of %d. Threshold = %.2f",
              __func__, running, children_count, threshold);
    return threshold > FORK_THRESHOLD;
}

int runPreforked() {
    int rv;

    setHandler(SIGCHLD, sigchld_handler);
    setHandler(SIGHUP,  sighup_handler_parent);
    setHandler(SIGTERM, sigterm_handler_parent);
    setHandler(SIGINT,  sigterm_handler_parent);

    // create lockfiles
    tmp_lockfd = mkstemp(tmp_lockfile);
    if(tmp_lockfd < 0) log_error("%s: mkstemp(appslck) failed: %s", __func__, strerror(errno));
    shr_lockfd = mkstemp(shr_lockfile);
    if(shr_lockfd < 0) log_error("%s: mkstemp(appshrd) failed: %s", __func__, strerror(errno));

    // create shared memory region
    pslots = (pslot_t*)mmap(NULL, CHILDREN_TABSIZE, PROT_READ|PROT_WRITE, MAP_ANON|MAP_SHARED, -1, 0);
    if(pslots == MAP_FAILED) log_error("%s: mmap failed: %s", __func__, strerror(errno));
    bzero(pslots, CHILDREN_TABSIZE);

    // Fast CGI staff
    rv = FCGX_Init();
    if(rv) log_error("%s: FCGX_Init failed: %s", __func__, strerror(rv));

    fcgi_socket = FCGX_OpenSocket(cpt->get<std::string>("common.fcgisocket", ":9090").c_str(),
                                  CHILDREN_HARDLIMIT);
    if(fcgi_socket < 0) log_error("%s: FCGX_OpenSocket failed: %s", __func__, strerror(errno));

    // main loop

    forkChildren();
    while(children_running > 0) {
        sleep(SLEEPTIME);
        if(needFork()) forkChildren();
    }

    log_warning("%s: shutdown in progress...", __func__);

    FCGX_ShutdownPending();
    close(fcgi_socket);

    munmap(pslots, CHILDREN_TABSIZE);

    close(shr_lockfd);
    close(tmp_lockfd);
    unlink(shr_lockfile);
    unlink(tmp_lockfile);
    
    return rv;
}










