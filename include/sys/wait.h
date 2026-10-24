#ifndef MINIX_SYS_WAIT_H
#define MINIX_SYS_WAIT_H

#include <sys/types.h>

/* Wait for a child process */
pid_t wait(int *status);

#endif /* MINIX_SYS_WAIT_H */
