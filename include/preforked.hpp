/**
 * @file   preforked.hpp
 * @author Gleb Semenov <gleb.semenov@gmail.com>
 * @date   Wed Jan 29 19:26:33 2014
 * 
 * @brief  Preforked process pool control utilities
 * 
 */

#ifndef __PREFORKED_HPP__
#define __PREFORKED_HPP__

#include <sys/types.h>
#include <sys/wait.h>
#include <sys/ipc.h>

// slot state
enum slotstate_t { ST_FREE, ST_IDLE, ST_BUSY, ST_SHUTDOWN };

struct pslot_t { // the element of control table stored in shared memory
    int         slotbusy;   // slot is occupied by child if field != 0
    slotstate_t childsts;   // slot status (empty, busy, idle, etc)
    pid_t       childpid;   // child process pid.
};

#define CHILDREN_HARDLIMIT 1024
#define CHILDREN_TABSIZE (CHILDREN_HARDLIMIT * sizeof(pslot_t))
#define CHILDREN_QUANTUM 16
#define FORK_THRESHOLD   0.79
#define SLEEPTIME 5
#define PARAMBUF_LENGTH 1024

int runPreforked();

#endif // #ifndef __PREFORKED_HPP__
    
    



