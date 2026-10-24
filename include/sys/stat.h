#ifndef MINIX_SYS_STAT_H
#define MINIX_SYS_STAT_H

#include <sys/types.h>
#include <stat.h>

/* File status */
int stat(const char *path, struct stat *buffer);
int fstat(int fd, struct stat *buffer);

/* File permissions */
int chmod(const char *path, mode_t mode);
mode_t umask(mode_t mask);

#endif /* MINIX_SYS_STAT_H */
