CC      = gcc
AS      = as
AR      = ar
RANLIB  = ranlib

ARCH    = -m32 -march=i386

CFLAGS  = $(ARCH) -std=c11 \
          -ffreestanding -fno-pie \
          -fno-stack-protector -fno-builtin \
          -fno-common \
          -Wall -Wextra \
          -I../include -I../h

ASFLAGS = --32

LIB     = libc.a

# All C sources in the library.
C_SOURCES := $(wildcard *.c)
C_OBJECTS := $(C_SOURCES:.c=.o)

# Assembly sources adapted for GNU as / i386.
ASM_SOURCES := crtso_i386.S head_i386.S end_i386.S \
               sendrec_i386.S brksize_i386.S \
               catchsig_i386.S csv_i386.S \
               getutil_i386.S setjmp_i386.S

ASM_OBJECTS := $(ASM_SOURCES:.S=.o)

.PHONY: all clean objects

all: $(LIB)

objects: $(C_OBJECTS) $(ASM_OBJECTS)

$(LIB): $(C_OBJECTS) $(ASM_OBJECTS)
	$(AR) rcs $@ $^
	$(RANLIB) $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

%_i386.o: %_i386.S
	$(AS) $(ASFLAGS) -o $@ $<

clean:
	rm -f *.o $(LIB)
