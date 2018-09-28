#include <sys/types.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <sys/file.h>
#include <fcntl.h>
#include <unistd.h>
#include <csignal>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <cerrno>
#include "fcgiapp.h"

#define CHILDREN_MAX 10

volatile int children_count = 0;

char tmp_lockfile[64];
int  fcgi_socket;

static void sigchld_handler(int sig) {
    int rv;
    pid_t pid;
	while ((pid = waitpid(-1, &rv, 0) > 0)) {
        children_count--;
	}
}

int run_child(int child_number) {
    int fd = open(tmp_lockfile, O_RDWR|O_CREAT, 0640);
    int rv = 0;
    if(fd < 0) {
        rv = errno;
        fprintf(stderr, "%s:%d: open error: %s\n", __func__, child_number, strerror(rv));
        return rv;
    }
    FCGX_Request request;

    flock(fd, LOCK_EX);
    rv = FCGX_InitRequest(&request, fcgi_socket, 0);
    flock(fd, LOCK_UN);
    if(rv) {
        fprintf(stderr, "%s:%d: FCGX_InitRequest error: %s\n",
                __func__, child_number, strerror(rv));
        return rv;
    }
    while(1) {
        flock(fd, LOCK_EX);
        rv = FCGX_Accept_r(&request);
        flock(fd, LOCK_UN);
        if(rv) {
            fprintf(stderr, "%s:%d: FCGX_Accept_r error: %s\n",
                    __func__, child_number, strerror(rv));
            return rv;
        }
        FCGX_PutS("Content-type: text/html\r\n\r\n", request.out);
        FCGX_PutS("<html>\r\n", request.out); 
        FCGX_PutS("<head>\r\n", request.out);
        FCGX_FPrintF(request.out, " <TITLE>fastcgi hellp #%d</TITLE>\r\n", child_number);
        FCGX_PutS("</head>\r\n", request.out);
        FCGX_PutS("<body>\r\n", request.out);
        for(int i = 0; request.envp[i]; i++)
            FCGX_FPrintF(request.out, " <p>%s</p>\r\n", request.envp[i]);
        FCGX_PutS("</body>\r\n", request.out);
        FCGX_PutS("</html>\r\n", request.out);
        FCGX_Finish_r(&request);
    }
    return rv;
}

int main(int ac, char **av) {
    int rv;
    struct sigaction act;

    snprintf(tmp_lockfile, sizeof(tmp_lockfile), "/tmp/fcgitest.%d", getpid());

	memset (&act, 0, sizeof(act));
	act.sa_handler = sigchld_handler;
    act.sa_flags = SA_RESTART | SA_NOCLDSTOP;
    
	if (sigaction(SIGCHLD, &act, 0)) {
		perror ("sigaction");
		return 1;
	}

    // Fast CGI staff
    rv = FCGX_Init();
    if(rv) {
        fprintf(stderr, "%s: FCGI_Init error: %s\n", __func__, strerror(rv));
        return rv;
    }
    fcgi_socket = FCGX_OpenSocket(av[1], CHILDREN_MAX); 
    
    for(children_count = 0; children_count < CHILDREN_MAX; children_count++) {
        pid_t pid = fork();
        if(pid == 0) run_child(children_count); // child
    }
    
    while(1) {
        rv = sleep(10);
        if(children_count <= 0) break;
    }
    
    unlink(tmp_lockfile);
    
    return 0;
}









