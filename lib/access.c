#include "../include/lib.h"

#include "call.h" // новое 

PUBLIC int access(name, mode)
char *name;
int mode;
{
  return callm3(FS, ACCESS, mode, name);

}
