/* /\* */
/*  * call.c — System call message helpers. */
/*  * */
/*  * Provides helpers for constructing IPC messages */
/*  * and sending them to system tasks. */
/*  *\/ */
/*  */
/* #include "../include/lib.h" */
/* #include "sendrec_i386.h" */
/*  */
/* #include "call.h" */
/*  */
/* int callm1(int proc, int syscallnr, int int1, int int2, int int3, char *ptr1, */
/*            char *ptr2, char *ptr3) { */
/*   /\* */
/*    * Send a message and get the response. */
/*    * M.m_type contains either a result (>= 0) */
/*    * or a negative error code. */
/*    *\/ */
/*  */
/*   M.m1_i1 = int1; */
/*   M.m1_i2 = int2; */
/*   M.m1_i3 = int3; */
/*  */
/*   M.m1_p1 = ptr1; */
/*   M.m1_p2 = ptr2; */
/*   M.m1_p3 = ptr3; */
/*  */
/*   return callx(proc, syscallnr); */
/* } */
/*  */
/* int callm3(int proc, int syscallnr, int int1, char *name) { */
/*  */
/*   /\* */
/*    * This form of system call is used for those calls that contain at most */
/*    * one integer parameter along with a string.  If the string fits in the */
/*    * message, it is copied there.  If not, a pointer to it is passed. */
/*    *\/ */
/*  */
/*   int k; */
/*   char *rp; */
/*  */
/*   k = len(name); */
/*  */
/*   M.m3_i1 = k; */
/*   M.m3_i2 = int1; */
/*   M.m3_p1 = name; */
/*  */
/*   rp = &M.m3_ca1[0]; */
/*  */
/*   if (k <= M3_STRING) { */
/*     while (k--) { */
/*       *rp++ = *name++; */
/*     } */
/*   } */
/*  */
/*   return callx(proc, syscallnr); */
/* } */
/*  */
/* int callx(int proc, int syscallnr) { */
/*   int k; */
/*  */
/*   /\* */
/*    * Send a message and get the response. */
/*    * M.m_type contains either a result (>= 0) */
/*    * or a negative error code (< 0). */
/*    *\/ */
/*  */
/*   M.m_type = syscallnr; */
/*  */
/*   k = sendrec(proc, &M); */
/*  */
/*   if (k != OK) { */
/*     return k; */
/*   } */
/*  */
/*   if (M.m_type < 0) { */
/*     errno = -M.m_type; */
/*     return -1; */
/*   } */
/*  */
/*   return M.m_type; */
/* } */
/*  */
/* int len(const char *s) { */
/*   /\* Return the length of a character string, including the 0 at the end. *\/ */
/*   int k = 0; */
/*  */
/*   while (*s++ != '\0') { */
/*     k++; */
/*   } */
/*  */
/*   return k + 1; /\* return length including the 0-byte at end *\/ */
/* } */



#include "../include/lib.h"
#include "sendrec_i386.h"
#include "call.h"

int callm1(int proc, int syscallnr,
           int int1, int int2, int int3,
           char *ptr1, char *ptr2, char *ptr3)
{
    M.m1_i1 = int1;
    M.m1_i2 = int2;
    M.m1_i3 = int3;

    M.m1_p1 = ptr1;
    M.m1_p2 = ptr2;
    M.m1_p3 = ptr3;

    return callx(proc, syscallnr);
}

int callm3(int proc, int syscallnr, int int1, char *name)
{
    int k;
    char *rp;

    k = len(name);

    M.m3_i1 = k;
    M.m3_i2 = int1;
    M.m3_p1 = name;

    rp = &M.m3_ca1[0];

    if (k <= M3_STRING) {
        while (k--)
            *rp++ = *name++;
    }

    return callx(proc, syscallnr);
}

int callx(int proc, int syscallnr)
{
    int result;

    M.m_type = syscallnr;

    result = sendrec(proc, &M);

    if (result != OK)
        return result;

    if (M.m_type < 0) {
        errno = -M.m_type;
        return -1;
    }

    return M.m_type;
}

int len(const char *s)
{
    int k = 0;

    while (*s++ != '\0')
        k++;

    return k + 1;
}
