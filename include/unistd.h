/*
 * unistd.h — user-space system-call prototypes for the i386 MINIX target.
 *
 * These are MINIX 1 signatures, not the hosted C library's exact POSIX
 * contract: byte counts are int and read/write return int.
 */
#ifndef MINIX_UNISTD_H
#define MINIX_UNISTD_H

int open(char *name, int mode);
int close(int fd);
int read(int fd, char *buffer, int nbytes);
int write(int fd, char *buffer, int nbytes);

int fork(void);
int sync(void);
int execn(char *name);

#endif /* MINIX_UNISTD_H */

/*
 * Historical header kept for comparison:
 *
 * int open(char *name, int mode);
 * int close(int fd);
 * int read(int fd, char *buffer, int nbytes);
 * int write(int fd, char *buffer, int nbytes);
 * int fork(void);
 * int wait(int *status);
 * int sync(void);
 * long lseek(int fd, long offset, int whence);
 */
