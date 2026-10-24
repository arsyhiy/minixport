#ifndef MINIX_STRINGS_H
#define MINIX_STRINGS_H

#include <stddef.h>

void bcopy(const void *src, void *dest, size_t n);
char *index(const char *str, int ch);
char *rindex(const char *str, int ch);

#endif /* MINIX_STRINGS_H */
