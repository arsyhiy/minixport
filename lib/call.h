/* call.h: header for System call message helpers */

#ifndef CALL_H
#define CALL_H

extern int errno; /* place where error numbers go */

// TODO: добавить коментарии которые обьесняют функции да вообще что делает call.c

int callm1(int proc, int syscallnr, int int1, int int2, int int3, char *ptr1, char *ptr2, char *ptr3);

int callm3(int proc, int syscallnr,
           int int1, char *name);

int callx(int proc, int syscallnr);

int len(const char *s);

#endif
