#ifndef MINIX_UTIME_H
#define MINIX_UTIME_H

#include <time.h>

struct utimbuf {
    time_t actime;
    time_t modtime;
};

int utime(const char *path, const struct utimbuf *times);

#endif /* MINIX_UTIME_H */
