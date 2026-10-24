/*
 * abort.c — Abort program execution.
 */

#include "abort.h"
#include "exit.h"

void abort(void)
{
  exit(99);
}
