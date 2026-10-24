#ifndef MINIX_STDDEF_H
#define MINIX_STDDEF_H

/* Fundamental types */
typedef unsigned int size_t;
typedef int ptrdiff_t;

/* Null pointer constant */
#ifndef NULL
#define NULL ((void *)0)
#endif

/* Offset of a member within a structure */
#define offsetof(type, member) \
    __builtin_offsetof(type, member)

#endif /* MINIX_STDDEF_H */
