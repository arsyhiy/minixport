/* /\* call.h: header for System call message helpers *\/ */
/*  */
/* #ifndef CALL_H */
/* #define CALL_H */
/*  */
/* extern int errno; /\* place where error numbers go *\/ */
/*  */
/* // TODO: добавить коментарии которые обьесняют функции да вообще что делает call.c */
/*  */
/* int callm1(int proc, int syscallnr, int int1, int int2, int int3, char *ptr1, char *ptr2, char *ptr3); */
/*  */
/* int callm3(int proc, int syscallnr, */
/*            int int1, char *name); */
/*  */
/* int callx(int proc, int syscallnr); */
/*  */
/* int len(const char *s); */
/*  */
/* #endif */


/*
 * call.h — Interfaces for MINIX system-call message helpers.
 *
 * callm1() builds a message with integer and pointer arguments.
 * callm3() builds a message containing a string argument.
 * callx() sends the message to a system task and processes the result.
 */

#ifndef MINIX_CALL_H
#define MINIX_CALL_H

extern int errno;

int callm1(
    int proc,
    int syscallnr,
    int int1,
    int int2,
    int int3,
    char *ptr1,
    char *ptr2,
    char *ptr3
);

int callm3(
    int proc,
    int syscallnr,
    int int1,
    char *name
);

int callx(int proc, int syscallnr);

/* Returns the string length including the terminating '\0'. */
int len(const char *s);

#endif /* MINIX_CALL_H */
