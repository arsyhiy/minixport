# Makefile for tools/init on i386.
#
# Builds only what init needs: the runtime entry object plus the small
# syscall/message helper objects from lib/.  There is no full libc.a here —
# init.c uses raw syscall stubs only, so it stays small enough for a MINIX
# image builder that works on 16-bit-sized binaries.
#
# Targets:
#   make all      — link tools/init (default)
#   make lib      — build lib/libinit.a and lib/head_i386.o only
#   make clean    — remove tools/init and generated objects
#
# After building, hand tools/init to your MINIX image builder (tools/build.c)
# along with the kernel, MM and FS binaries.  Boot the resulting image in
# QEMU with:
#   qemu-system-i386 -drive format=raw,file=<image>,if=floppy
# or attach it as the first hard disk depending on how the image is packed.

CC      = gcc
LD      = ld
PYTHON  = python3 -I
SCRIPTS = scripts
BOOTBIN = bootblok.bin
DEMO_FS  = demo/demo.fs
DEMO_IMG = demo/demo.img
CFLAGS  = -m32 -march=i386 -std=c11 \
          -ffreestanding -fno-pie -fno-pic \
          -fno-stack-protector -fno-builtin \
          -fno-common -Wall -Wextra \
          -I../include -I../h
ASFLAGS = -m32 -march=i386 -ffreestanding -fno-pie
LDFLAGS = -m elf_i386 -nostdlib --entry=_start

INIT_LIB = ../lib/libinit.a
HEAD_O   = ../lib/runtime/head_i386.o

.PHONY: all lib clean bootblok demo-fs demo-image demo-fsck

all: init

bootblok: $(BOOTBIN)

$(BOOTBIN): $(SCRIPTS)/bootblok.py
	$(PYTHON) $< $@
	test "$$(wc -c < $@)" -eq 512
	test "$$(od -An -tx1 -j510 -N2 $@ | tr -d ' \\n')" = 55aa

demo-fs: $(DEMO_FS)

$(DEMO_FS): $(SCRIPTS)/mkfs.py demo/files.json demo/hello.txt init
	$(PYTHON) $(SCRIPTS)/mkfs.py $@ demo/files.json

demo-image: $(DEMO_IMG)

$(DEMO_IMG): $(BOOTBIN) $(SCRIPTS)/build.py init $(DEMO_FS)
	$(PYTHON) $(SCRIPTS)/build.py $@ $(BOOTBIN) init init demo.fs $(DEMO_FS)

demo-fsck: $(DEMO_FS) $(SCRIPTS)/fsck.py
	$(PYTHON) $(SCRIPTS)/fsck.py $(DEMO_FS)

init: init.o $(HEAD_O) $(INIT_LIB)
	$(LD) $(LDFLAGS) -o $@ $(HEAD_O) init.o $(INIT_LIB)
	@echo init done.

init.o: init.c
	$(CC) $(CFLAGS) -c init.c -o $@

$(HEAD_O) $(INIT_LIB):
	$(MAKE) -C ../lib

lib:
	$(MAKE) -C ../lib

clean:
	rm -f init init.o

# ---------------------------------------------------------------------------
# Original MINIX 1 tools makefile, kept for reference.
# It used the compiler driver to link a full libc.a and relied on the host's
# own crt0/head.o — that no longer matches a freestanding i386 target, so it
# is commented out.  Uncomment to compare; do not mix with the rules above.
#
# l=../lib
# CFLAGS = -O -DPCIX
#
# all:
# 	make init
# 	# make bootblok
# 	# make build
# 	# make mkfs
# 	# make fsck
#
# init:	$l/libc.a init.o $l/head.o
# 	ld -o init  $l/head.o init.o $l/libc.a  $l/end.o
# 	@echo init done.
#
# bootblok:	bootblok.s
# 	as bootblok.s
# 	@strip a.out
# 	@dd if=a.out of=bootblok bs=16w skip=1 count=16 2>/dev/null
# 	@rm a.out
# 	@echo bootblok done.
#
# build:	build.o
# 	cc -o build build.o
# 	@echo build done.
#
# mkfs:	mkfs.o
# 	cc -o mkfs mkfs.o
# 	@echo mkfs done.
# mkfs.o:	mkfs.c
# 	cc -c -O -DUNIX mkfs.c
#
# fsck:	fsck.o fsck1.o
# 	@ld -o fsck  fsck1.o fsck.o $l/libc.a
# 	@echo fsck done.
# fsck1.o:	fsck1.s
# 	as -o fsck1.o fsck1.s
#
# image:	 build bootblok ../kernel/kernel ../mm/mm ../fs/fs init fsck
# 	@build bootblok ../kernel/kernel ../mm/mm ../fs/fs init fsck  image
# 	cp image /dev/fd0
#
# usr:	mkfs proto.usr
# 	mkfs -l usr proto.usr >log.usr
# 	cp usr /dev/fd0
#
# ram:	mkfs proto.ram rc
# 	mkfs -l ram proto.ram >log.ram
# 	cp ram /dev/fd0
# ---------------------------------------------------------------------------
