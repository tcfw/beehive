#ifndef _ERRNO_H
#define _ERRNO_H

#define ERRNOSYS (1)	 // no system call exists
#define ERRINTR (2)		 // syscall was interrupted
#define ERRACCESS (3)	 // access forbidden
#define ERRFAULT (4)	 // general fault
#define ERRNOPROC (5)	 // no matching process
#define ERRSIZE (6)		 // bad size
#define ERREXISTS (7)	 // resource already exists
#define ERREXHAUSTED (8) // resource limits exhausted
#define ERRINUSE (9)	 // resource in use
#define ERRAGAIN (10)	 // attempt syscall again
#define ERRINVAL (11)	 // invalid parameters
#define ERRNOMEM (12)	 // no memory available
#define ERRNOENT (13)	 // no such resource

#endif