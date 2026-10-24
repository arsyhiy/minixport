#ifndef MINIX_SYS_TIMES_H
#define MINIX_SYS_TIMES_H

#include <sys/types.h>

/* Process execution times */
struct tms {
    long tms_utime;
    long tms_stime;
    long tms_cutime;
    long tms_cstime;
};

long times(struct tms *buffer);

#endif /* MINIX_SYS_TIMES_H */
