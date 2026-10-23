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

/* extern int begsig();		/\* interrupts all vector here *\/ */

#endif /* MINIX_LIB_H */
