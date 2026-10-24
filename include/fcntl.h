#ifndef MINIX_FCNTL_H
#define MINIX_FCNTL_H

/* File access modes */
#define O_RDONLY    0
#define O_WRONLY    1
#define O_RDWR      2

/* File creation and status flags */
#define O_CREAT     0100
#define O_EXCL      0200
#define O_NOCTTY    0400
#define O_TRUNC     01000
#define O_APPEND    02000
#define O_NONBLOCK  04000

/* Open a file */
int open(const char *path, int flags, ...);

/* Create or truncate a file */
int creat(const char *path, unsigned int mode);

#endif /* MINIX_FCNTL_H */
