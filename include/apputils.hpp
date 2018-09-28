#ifndef __APPUTILS_HPP__
#define __APPUTILS_HPP__

/**
 * @file   apputils.hpp
 * @author Gleb Semenov <gleb.semenov@gmail.com>
 * @date   Fri Oct 25 12:54:03 2013
 * 
 * @brief  utility function prototypes
 */
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <unistd.h>
#include <fcntl.h>
#include <cstdlib>
#include <csignal>
#include <cstdarg>
#include <cstring>
#include <cstdio>
#include <cerrno>
#include <syslog.h>

#define checkPoint(x) fprintf(STDERR, "Checkpoint #%d\n", x)

void daemonize();
int write_pid(const char* _pidfile);

/* logging via syslog */
void init_syslog(int facility, const char* ident, bool debug_mode = false);
void log_debug(const char *format, ...);
void log_error(const char *format, ...);
void log_warning(const char *format, ...);
void log_message(const char *format, ...);

/* unix system service wrappers */

pid_t Wait(int *iptr);
pid_t Waitpid(pid_t pid, int *iptr, int options);
size_t Write(int fd, const void *ptr, size_t nbytes);
int Unlink(const char *path);
ssize_t Read(int fd, void *ptr, size_t nbytes);
int Accept(int fd, struct sockaddr *sa, socklen_t *addrlen);
int Getpeername(int fd, struct sockaddr *sa, socklen_t *addrlen);
int Getsockname(int fd, struct sockaddr *sa, socklen_t *addrlen);
int Listen(int fd, int backlog);
ssize_t Recv(int fd, void *ptr, size_t nbytes, int flags);
int Select(int nfds, fd_set *readfds, fd_set *writefds, fd_set *exceptfds, struct timeval *timeout);
int Send(int fd, const void *ptr, size_t nbytes, int flags);
int Setsockopt(int fd, int level, int optname, const void *optval, socklen_t optlen);
int Socket(int family, int type, int protocol);

#endif // #ifndef __APPUTILS_HPP__





