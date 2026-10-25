/*
 * lib.h — internal MINIX system-library declarations.
 *
 * This header is the bridge between the public UNIX-style API and the
 * kernel IPC ABI.  User programs normally include unistd.h/stdlib.h; the
 * objects in lib/ include lib.h so they can construct message structures and
 * call MM/FS through sendrec().
 */
#ifndef MINIX_LIB_H
#define MINIX_LIB_H

#include "../h/const.h"
#include "../h/type.h"
#include "../h/error.h"
#include "../h/callnr.h"

/* Shared IPC buffer. Defined by lib/syscall/message.o. */
extern message M;

/* Last system error. */
extern int errno;

/* Process numbers used by system calls. */
#define MM 0
#define FS 1

/* Build and send a three-integer, three-pointer message. */
int callm1(int proc, int syscallnr,
           int int1, int int2, int int3,
           char *ptr1, char *ptr2, char *ptr3);

/* Build an M3 message containing an integer and a pathname. */
int callm3(int proc, int syscallnr, int int1, char *name);

/* Send the current message and translate the reply into a UNIX result. */
int callx(int proc, int syscallnr);

/* Return strlen(name) + 1, including the terminating NUL. */
int len(const char *s);

/* Assembly entry point for kernel/user IPC. */
int sendrec(int srcdest, message *msg);

/* Execute NAME with no argv or environment. */
int execn(char *name);

#endif /* MINIX_LIB_H */

/*
 * Original header text, retained to show what changed for the i386 port:
 *
 * #include "../h/const.h"
 * #include "../h/type.h"
 * #include "../h/error.h"
 * #include "../h/callnr.h"
 * extern message M;
 * extern int errno;
 * #define MM 0
 * #define FS 1
 * extern int begsig();
 *
 * The port adds explicit prototypes so GCC's C11 frontend can compile the
 * freestanding sources without K&R function declarations.
 */
