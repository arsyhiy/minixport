/* #ifndef MINIX_UNISTD_H */
/* #define MINIX_UNISTD_H */
/*  */
/* #include "stddef.h" */
/* #include "stdint.h" */ /* #include "sys/types.h" */
/*  */
/* /\* File I/O *\/ */
/* int access(const char *path, int mode); // access.c */
/* int close(int fd); */
/* int dup(int fd); */
/* int dup2(int oldfd, int newfd); */
/* off_t lseek(int fd, off_t offset, int whence); */
/* int pipe(int fd[2]); */
/* ssize_t read(int fd, void *buffer, size_t count); */
/* ssize_t write(int fd, const void *buffer, size_t count); */
/*  */
/* /\* Process control *\/ */
/* int fork(void); */
/* int execve(const char *path, char *const argv[], char *const envp[]); */
/* int pause(void); */
/* unsigned int sleep(unsigned int seconds); */
/* unsigned int alarm(unsigned int seconds); */
/* int sync(void); */
/*  */
/* /\* File system *\/ */
/* int chdir(const char *path); */
/* int chroot(const char *path); */
/* int link(const char *oldpath, const char *newpath); */
/* int unlink(const char *path); */
/*  */
/* /\* User and group IDs *\/ */
/* uid_t getuid(void); */
/* uid_t geteuid(void); */
/* gid_t getgid(void); */
/* gid_t getegid(void); */
/* int setuid(uid_t uid); */
/* int setgid(gid_t gid); */
/*  */
/* /\* Terminal *\/ */
/* int isatty(int fd); */
/*  */
/* /\* MINIX-specific / legacy interfaces *\/ */
/* int brk(void *addr); */
/* void *sbrk(intptr_t increment); */
/*  */
/* #endif /\* MINIX_UNISTD_H *\/ */


#ifndef MINIX_UNISTD_H
#define MINIX_UNISTD_H

int open(char *name, int mode);
int close(int fd);

int read(int fd, char *buffer, int nbytes);
int write(int fd, char *buffer, int nbytes);

int fork(void);
int wait(int *status);
int sync(void);

long lseek(int fd, long offset, int whence);

#endif /* MINIX_UNISTD_H */
