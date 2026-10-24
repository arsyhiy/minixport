#ifndef MINIX_STRING_H
#define MINIX_STRING_H

#include <stddef.h>

char *strcat(char *dest, const char *src);
int strcmp(const char *s1, const char *s2);
char *strcpy(char *dest, const char *src);
size_t strlen(const char *str);
char *strncat(char *dest, const char *src, size_t n);
int strncmp(const char *s1, const char *s2, size_t n);
char *strncpy(char *dest, const char *src, size_t n);

#endif /* MINIX_STRING_H */
