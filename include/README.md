# MINIX user-space headers
#
# Headers in this directory are the public i386 user ABI.  Kernel-private
# headers remain in ../h and are included explicitly by the small system
# library.  User programs should include these paths with:
#
#   -Iinclude -Ih
#
# Common entry points:
#   #include <unistd.h>       read, write, open, close, fork, sync
#   #include <stdlib.h>       exit, abort
#   #include <sys/wait.h>     wait
#   #include <lib.h>          MINIX IPC helpers and execn
#
# The headers intentionally expose the historical MINIX interfaces rather
# than pretending to be a hosted POSIX libc.  In particular, file descriptors
# and syscall results are int-sized on the i386 target.
#
# Internal implementation headers such as h/type.h and h/callnr.h are not
# duplicated here: lib/ includes them through lib.h so message layout and
# syscall numbers have one source of truth.
