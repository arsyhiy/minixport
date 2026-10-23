/*
 * lib.h — Common library declarations.
 *
 * Provides shared definitions and interfaces
 * used by the MINIX system library.
 */

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

int callm1(int proc, int syscallnr,
           int i1, int i2, int i3,
           char *p1, char *p2, char *p3);

int callm3(int proc, int syscallnr,
           char *name, int int1);

int callx(int proc, int syscallnr);
int len(const char *s);
/* extern int begsig();		/\* interrupts all vector here *\/ */

#endif /* MINIX_LIB_H */
