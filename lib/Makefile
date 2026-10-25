# i386 library fragments used by tools/init.
#
# This is intentionally not a complete libc.  Keep the object list explicit:
# changing one syscall implementation must not silently pull every legacy
# library directory into an init image.

CC     ?= gcc
AR     ?= ar
RANLIB ?= ranlib

ARCH   = -m32 -march=i386
CFLAGS = $(ARCH) -std=c11 -ffreestanding -fno-pie -fno-pic \
         -fno-stack-protector -fno-builtin -fno-common \
         -Wall -Wextra -I../include -I../h
ASFLAGS = $(ARCH) -ffreestanding -fno-pie

LIB = libinit.a

# Only the functions called by tools/init.c, plus the IPC entry point.
C_OBJECTS = syscall/call.o syscall/close.o syscall/errno.o syscall/message.o \
            syscall/open.o syscall/read.o syscall/sync.o syscall/write.o \
            process/exec.o process/exit.o process/fork.o process/wait.o
ASM_OBJECTS = syscall/sendrec_i386.o
STARTUP = runtime/head_i386.o
OBJECTS = $(C_OBJECTS) $(ASM_OBJECTS)

.PHONY: all clean startup

all: $(LIB) $(STARTUP)

$(LIB): $(OBJECTS)
	$(AR) rcs $@ $^
	$(RANLIB) $@

syscall/%.o: syscall/%.c
	$(CC) $(CFLAGS) -c $< -o $@

process/%.o: process/%.c
	$(CC) $(CFLAGS) -c $< -o $@

syscall/sendrec_i386.o: syscall/sendrec_i386.s
	$(CC) $(ASFLAGS) -c $< -o $@

runtime/head_i386.o: runtime/head_i386.s
	$(CC) $(ASFLAGS) -c $< -o $@

startup: $(STARTUP)

clean:
	rm -f $(OBJECTS) $(STARTUP) $(LIB)

# ---------------------------------------------------------------------------
# Previous experimental Makefile, retained as documentation.
# It listed globals.c and open.c as if they lived directly in lib/, although
# the sources were moved into subdirectories.  It also built a broad libc.a.
#
# C_SOURCES := call.c close.c exec.c exit.c fork.c \
#              globals.c open.c read.c sync.c wait.c write.c
# ASM_SOURCES := sendrec_i386.s
# STARTUP_OBJECTS := head_i386.o
# all: libc.a startup
# libc.a: $(C_OBJECTS) $(ASM_OBJECTS)
# 	$(AR) rcs $@ $^
# startup: $(STARTUP_OBJECTS)
# ---------------------------------------------------------------------------
