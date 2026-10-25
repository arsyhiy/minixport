/*
 * access.c: Check file accessibility.

 * This file implements access(), which requests the file system
 * to check access permissions for a specified file.
 */

#include "lib.h"
#include "fcntl.h"

/* int access(char *name, int mode) */
int access(const char *path, int mode)
{
  return callm3(FS, ACCESS, mode, path);
}
