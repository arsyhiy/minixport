#ifndef MINIX_LIB_H
#define MINIX_LIB_H

#include "../h/const.h"
#include "../h/type.h"
#include "../h/error.h"
#include "../h/callnr.h"

extern message M;
extern int errno;

#define MM 0
#define FS 1

/* Internal library routines. */
int callm1(int proc, int syscallnr,
           int int1, int int2, int int3,
           char *ptr1, char *ptr2, char *ptr3);

int callm3(int proc, int syscallnr,
           int int1, char *name);

int callx(int proc, int syscallnr);

int len(const char *s);

/* Signal trampoline implemented in assembly. */
int begsig(void);




int execn(char *name);
int execve(char *name, char *argv[], char *envp[]);

#endif /* MINIX_LIB_H */

/* /\* */
/*  * lib.h — Common library declarations. */
/*  * */
/*  * Provides shared definitions and interfaces */
/*  * used by the MINIX system library. */
/*  *\/ */
/*  */
/* #ifndef MINIX_LIB_H */
/* #define MINIX_LIB_H */
/*  */
/* #include "../h/const.h" */
/* #include "../h/type.h" */
/* #include "../h/error.h" */
/* #include "../h/callnr.h" */
/*  */
/* extern message M; */
/* extern int errno; */
/*  */
/* #define MM 0 */
/* #define FS 1 */
/*  */
/* /\* extern int begsig();		/\\* interrupts all vector here *\\/ *\/ */
/*  */
/* #endif /\* MINIX_LIB_H *\/ */
