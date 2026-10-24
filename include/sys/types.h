#ifndef MINIX_SYS_TYPES_H
#define MINIX_SYS_TYPES_H

#include "../include/stddef.h"
#include "../include/stdint.h"

/* File sizes and offsets */
typedef long off_t;

/* Process and user identifiers */
typedef int pid_t;
typedef unsigned int uid_t;
typedef unsigned int gid_t;

/* Signed result of read() and write() */
typedef long ssize_t;

/* File mode and device identifiers */
typedef unsigned int mode_t;
typedef unsigned int dev_t;
typedef unsigned int ino_t;

#endif /* MINIX_SYS_TYPES_H */
