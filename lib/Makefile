# CC      = gcc
# AR      = ar
# RANLIB  = ranlib
# 
# ARCH    = -m32 -march=i386
# 
# CFLAGS  = $(ARCH) -std=c11 \
#           -ffreestanding -fno-pie \
#           -fno-stack-protector -fno-builtin \
#           -fno-common \
#           -Wall -Wextra \
#           -I../include -I../h
# 
# LIB     = libc.a
# 
# # All C sources in the library.
# C_SOURCES := $(wildcard *.c)
# C_OBJECTS := $(C_SOURCES:.c=.o)
# 
# # # Assembly sources providing library functions.
# # ASM_SOURCES := sendrec_i386.S brksize_i386.S \
# #                catchsig_i386.S csv_i386.S \
# #                getutil_i386.S setjmp_i386.S
# # 
# # ASM_OBJECTS := $(ASM_SOURCES:.S=.o)
# # 
# # # Startup and linker-support objects.
# # STARTUP_SOURCES := crtso_i386.S head_i386.S end_i386.S
# # STARTUP_OBJECTS := $(STARTUP_SOURCES:.S=.o)
# 
# 
# # Minimal library required by init, without signal support for now.
# C_SOURCES := call.c close.c exec.c exit.c fork.c \
#              globals.c open.c read.c sync.c wait.c write.c
# C_OBJECTS := $(C_SOURCES:.c=.o)
# 
# # i386 system-call entry.
# ASM_SOURCES := sendrec_i386.s
# ASM_OBJECTS := $(ASM_SOURCES:.s=.o)
# 
# # Startup objects remain separate.
# STARTUP_SOURCES := crtso.s head.s end.s
# STARTUP_OBJECTS := $(STARTUP_SOURCES:.s=.o)
# 
# .PHONY: all clean objects startup
# 
# all: $(LIB) startup
# 
# objects: $(C_OBJECTS) $(ASM_OBJECTS) $(STARTUP_OBJECTS)
# 
# startup: $(STARTUP_OBJECTS)
# 
# $(LIB): $(C_OBJECTS) $(ASM_OBJECTS)
# 	$(AR) rcs $@ $^
# 	$(RANLIB) $@
# 
# # %.o: %.c
# # 	$(CC) $(CFLAGS) -c $< -o $@
# 
# %.o: %.s
# 	$(CC) $(CFLAGS) -c $< -o $@
# 
# %_i386.o: %_i386.S
# 	$(CC) $(CFLAGS) -c $< -o $@
# 
# clean:
# 	rm -f *.o $(LIB)



CC      = gcc
AR      = ar
RANLIB  = ranlib

ARCH    = -m32 -march=i386
CFLAGS  = $(ARCH) -std=c11 \
          -ffreestanding -fno-pie \
          -fno-stack-protector -fno-builtin \
          -fno-common -Wall -Wextra \
          -I../include -I../h

LIB = libc.a

C_SOURCES := call.c close.c exec.c exit.c fork.c \
             globals.c open.c read.c sync.c wait.c write.c
C_OBJECTS := $(C_SOURCES:.c=.o)

ASM_SOURCES := sendrec_i386.s
ASM_OBJECTS := $(ASM_SOURCES:.s=.o)

STARTUP_OBJECTS := head_i386.o

.PHONY: all clean startup

all: $(LIB) startup

$(LIB): $(C_OBJECTS) $(ASM_OBJECTS)
	$(AR) rcs $@ $^
	$(RANLIB) $@

startup: $(STARTUP_OBJECTS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

%.o: %.s
	$(CC) $(CFLAGS) -c $< -o $@

head_i386.o: minix/head_i386.s
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f *.o $(LIB)
