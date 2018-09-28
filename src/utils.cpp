/**
 * @file   utils.cpp
 * @author Gleb Semenov <gleb.semenov@gmail.com>
 * @date   Fri Oct 25 12:52:47 2013
 *
 * @brief  different small utility functions
 */

#include "config.h"
#ifdef HAVE_LIBBSD
#include <bsd/string.h>
#else
#include <cstring>
#endif
#include "apputils.hpp"

#define LOGBUFF_SIZE 256

static bool debug_mode = true; // altered by the init_syslog function

/**
 * @fn void daemonize(void)
 * @brief it makes me be a daemon
 * @param none
 * @return none
 */
void daemonize() {
    pid_t pid;
    if((pid = fork()) != 0 ) exit(0); // Parent goes down
    setsid(); // be a session leader

    signal(SIGQUIT, SIG_IGN);
    signal(SIGTTIN, SIG_IGN);
    signal(SIGTTOU, SIG_IGN);
    signal(SIGTSTP, SIG_IGN);

    if((pid = fork()) != 0 ) exit(0); // second fork, first child goes down

    // And second child continue
    if(chdir("/") != 0) exit(errno);
    umask(0);

    // reopen channels via old BSD-style trick
    int nullfd = open("/dev/null", O_RDWR, 0);
    close(0), dup2(nullfd, 0);
    close(1), dup2(nullfd, 1);
    close(2), dup2(nullfd, 2);
    close(nullfd);
}

/**
 * @fn int write_pid(const char *pidfile)
 * @brief writes a pid file in secure way, paranoid version
 * @return errno
 */
int write_pid(const char *pidfile) {
    int file;
    char pid[32];
    struct stat lstatInfo;

    snprintf(pid, sizeof(pid), "%d", getpid());

    if(lstat(pidfile, &lstatInfo) == -1) {
        // If the lstat() failed for reasons other than the file not
        // existing, return a file open error
        if(errno != ENOENT)
            log_error("Failed to open file %s : %s", pidfile, strerror(errno));

        // The file doesn't exist, create it with O_EXCL to make sure an
        // attacker can't slip in a file between the lstat() and open()
        file = open(pidfile, O_CREAT | O_EXCL | O_RDWR, 0600);
        if(file < 0) return errno;
    }
    else {
        struct stat fstatInfo;

        // Open an existing file
        file = open( pidfile, O_RDWR );
        if(file == -1) log_error("Failed open file %s : %s", pidfile, strerror(errno));

        // fstat() the opened file and check that the file mode bits and
        // inode and device match
        if(fstat(file, &fstatInfo ) == -1 ||
           lstatInfo.st_mode != fstatInfo.st_mode ||
           lstatInfo.st_ino != fstatInfo.st_ino ||
           lstatInfo.st_dev != fstatInfo.st_dev)
        {
            close(file);
            log_error("File %s changed, possible race attack, exiting...", pidfile);
        }

        // If the above check was passed, we know that the lstat() and
        // fstat() were done to the same file.  Now check that there's
        // only one link, and that it's a normal file(this isn't
        // strictly necessary because the fstat() vs lstat() st_mode
        // check would also find this)
        if(fstatInfo.st_nlink > 1 || !S_ISREG( lstatInfo.st_mode ) ) {
            close( file );
            log_error("File %s must be normal file( possible simlink attack ), exiting ...", pidfile);
        }
    }
    if(ftruncate(file, 0) < 0) return errno;
    if((size_t)write(file, pid, strlen(pid)) != strlen(pid)) return errno;
    return close(file);
}

/**
 * @fn int init_syslog(boost::property_tree::ptree& config)
 * @brief initializes syslog facility, initializes debug mode flag
 * @param int facility -- logging facility (0..7)
 * @param const char* ident -- log identifier
 * @param bool debug_mode -- debug mode flag
 * @return 0 if no error, errno otherwise
 */
void init_syslog(int facility, const char* ident, bool debug_flag) {
    static constexpr int local_facilities[] =
        {LOG_LOCAL0, LOG_LOCAL1, LOG_LOCAL2, LOG_LOCAL3,
         LOG_LOCAL4, LOG_LOCAL5, LOG_LOCAL6, LOG_LOCAL7};
    if(facility < 0 || facility > 7) facility = 7;
    debug_mode = debug_flag;
    openlog(ident, LOG_PID|LOG_NDELAY, local_facilities[facility]);
}

/**
 * @fn static inline void vlog(int priority, const char *severity, const char *format, va_list args)
 * @brief common logging routine
 * @return none
 */
static inline void vlog(int priority, const char *severity, const char *format, va_list args) {
    char buf[LOGBUFF_SIZE];
    strncpy(buf, severity, sizeof buf);
    strlcat(buf, format,   sizeof buf);
    vsyslog(priority, buf, args);
}

/**
 * @fn void log_debug(const char *format, ...)
 * @brief Print debug message to stdout or syslog(only if debug flag is set)
 * @param const char *format, ...  - printf-style format and arguments
 * @return none
 */
void log_debug(const char *format, ...) {
    static const char severity[] = "ASDEBUG: ";
    va_list args;

    if(!debug_mode) return;

    va_start(args, format);
    vlog(LOG_DEBUG, severity, format, args);
    va_end(args);
}

/**
 * @fn void log_error(const char *format, ...)
 * @brief Print error message to stdout or syslog and terminates program
 * @param const char *format, ...  - printf-style format and arguments
 * @return none
 */
void log_error(const char *format, ...) {
    static const char severity[] = "ASERROR: ";
    va_list args;
    va_start(args, format);
    vlog(LOG_ERR, severity, format, args);
    va_end(args);
    syslog(LOG_ERR, "ASERROR: server terminated by critical error");
    exit(1);
}

/**
 * @fn void log_warning(const char *format, ...)
 * @brief Print warning message to stdout or syslog
 * @param const char *format, ...  - printf-style format and arguments
 * @return none
 */
void log_warning(const char *format, ...) {
    static const char severity[] = "ASWARN: ";
    va_list args;
    va_start(args, format);
    vlog(LOG_WARNING, severity, format, args);
    va_end(args);
}

/**
 * @fn void log_warning(const char *format, ...)
 * @brief prints information message to stdout or syslog
 * @param const char *format, ...  - printf-style format and arguments
 * @return none
 */
void log_message(const char *format, ...) {
    static const char severity[] = "ASINFO: ";
    va_list args;
    va_start(args, format);
    vlog(LOG_INFO, severity, format, args);
    va_end(args);
}
