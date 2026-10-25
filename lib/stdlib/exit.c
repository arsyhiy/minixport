#include "../include/lib.h"
#include "call.h"
#include "stdlib.h"

int exit(int status)
{
    return callm1(MM, EXIT, status, 0, 0,
                  NIL_PTR, NIL_PTR, NIL_PTR);
}

/* #include "../include/lib.h" */
/* #include "call.h" */
/*  */
/*  */
/* #include "stdlib.h" */
/*  */
/* int exit(int status) */
/* { */
/*     return callm1(MM, EXIT, status, 0, 0, */
/*                   NIL_PTR, NIL_PTR, NIL_PTR); */
/* } */
