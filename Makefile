# Makefile — MINIX 1 port, M1 milestone image.
#
# Builds a BIOS-bootable floppy image in the original MINIX module order:
#
#     bootblok | kernel | mm | fs | init | rootfs
#
# M1 intentionally keeps MM/FS/INIT as tiny position-holder stubs while the
# real 16-bit sources are being ported.  The image is assembled by
# tools/scripts/build.py, which also writes a small module table into the
# last words of the boot sector (like tools/build.c did for a.out binaries).
#
# Targets:
#   make all        build build/image/minix.img (default)
#   make run        boot the image in QEMU (floppy, SDL display)
#   make run-serial boot with serial console on stdio, no display
#   make clean      remove build/
#
# Requires: gcc(-m32), binutils, python3, qemu-system-i386.

CC      = gcc
AS      = as
LD      = ld
OBJCOPY = objcopy
PYTHON  = python3 -I

BUILD   = build/m1
OBJ     = $(BUILD)/obj
IMAGE   = $(BUILD)/minix.img

CFLAGS  = -m32 -ffreestanding -fno-pie -fno-pic -fno-stack-protector \
          -fno-builtin -O2 -Wall -Wextra
ASFLAGS = --32
KDIR    = kernel/x86_64
TOOLS   = tools
SCRIPTS = $(TOOLS)/scripts

BOOTBLK = $(BUILD)/bootblok.bin
KERNEL  = $(BUILD)/kernel.bin
MM      = $(BUILD)/mm.bin
FS      = $(BUILD)/fs.bin
INITMOD = $(BUILD)/init.bin
ROOTFS  = $(BUILD)/rootfs.img

QEMU    = qemu-system-i386
QFLAGS  = -drive format=raw,file=$(IMAGE),if=floppy \
          -serial file:$(BUILD)/serial.log -monitor none

.PHONY: all run run-serial clean
all: $(IMAGE)

# ---- boot sector ------------------------------------------------------------

$(BUILD):
	mkdir -p $(BUILD) $(OBJ)

$(BUILD)/bootblok.o: $(TOOLS)/bootblok_i386.s | $(BUILD)
	$(AS) $(ASFLAGS) -o $@ $<

$(BOOTBLK): $(BUILD)/bootblok.o
	$(LD) -m elf_i386 -Ttext 0x7C00 --oformat binary -o $@ $<
	@size=$$(wc -c < $@); \
	if [ $$size -gt 510 ]; then \
	    echo "bootblok: $$size bytes > 510 — shrink the loader"; exit 1; fi
	$(PYTHON) -c "import sys; d=open('$@','rb').read(); \
	    open('$@','wb').write(d + bytes(510-len(d)) + b'\\x55\\xaa')"
	@echo "bootblok: $$(wc -c < $@) bytes, signature OK"

# ---- flat module binaries ----------------------------------------------------
# Each module is linked at its fixed physical address and stripped to a flat
# binary, so byte 0 of the .bin is the module's entry point.

$(OBJ)/kernel_boot.o: $(KDIR)/boot.S | $(BUILD)
	$(CC) $(CFLAGS) -c -o $@ $<

$(OBJ)/kernel.o: $(KDIR)/kernel.c | $(BUILD)
	$(CC) $(CFLAGS) -c -o $@ $<

$(KERNEL): $(OBJ)/kernel_boot.o $(OBJ)/kernel.o $(KDIR)/linker.ld
	$(LD) -m elf_i386 -nostdlib -T $(KDIR)/linker.ld \
	      --oformat binary -o $@ $(OBJ)/kernel_boot.o $(OBJ)/kernel.o

$(OBJ)/mm.o: $(KDIR)/mm.S | $(BUILD)
	$(CC) $(CFLAGS) -c -o $@ $<

$(MM): $(OBJ)/mm.o $(KDIR)/linker_mm.ld
	$(LD) -m elf_i386 -nostdlib -T $(KDIR)/linker_mm.ld \
	      --oformat binary -o $@ $(OBJ)/mm.o

$(OBJ)/fs.o: $(KDIR)/fs.S | $(BUILD)
	$(CC) $(CFLAGS) -c -o $@ $<

$(FS): $(OBJ)/fs.o $(KDIR)/linker_fs.ld
	$(LD) -m elf_i386 -nostdlib -T $(KDIR)/linker_fs.ld \
	      --oformat binary -o $@ $(OBJ)/fs.o

$(OBJ)/init.o: $(KDIR)/init.S | $(BUILD)
	$(CC) $(CFLAGS) -c -o $@ $<

$(INITMOD): $(OBJ)/init.o $(KDIR)/linker_init.ld
	$(LD) -m elf_i386 -nostdlib -T $(KDIR)/linker_init.ld \
	      --oformat binary -o $@ $(OBJ)/init.o

# ---- MINIX v1 root file system ------------------------------------------------

$(ROOTFS): $(SCRIPTS)/mkfs.py $(TOOLS)/proto.m1 | $(BUILD)
	$(PYTHON) $(SCRIPTS)/mkfs.py $@ $(TOOLS)/proto.m1
	$(PYTHON) $(SCRIPTS)/fsck.py $@

# ---- image --------------------------------------------------------------------

$(IMAGE): $(BOOTBLK) $(KERNEL) $(MM) $(FS) $(INITMOD) $(ROOTFS) \
          $(SCRIPTS)/build.py
	$(PYTHON) $(SCRIPTS)/build.py $@ $(BOOTBLK) $(KERNEL) $(MM) $(FS) \
	    $(INITMOD) $(ROOTFS)

# ---- run -----------------------------------------------------------------------

run: $(IMAGE)
	$(QEMU) $(QFLAGS) -display gtk

run-serial: $(IMAGE)
	$(QEMU) -drive format=raw,file=$(IMAGE),if=floppy \
	        -serial stdio -display none -monitor none

clean:
	rm -rf build

# ==============================================================================
# Everything below is the previous experimental Makefile, kept for reference.
# It booted the standalone Multiboot2 terminal demo through GRUB/OVMF and is
# superseded by the BIOS boot-chain targets above.  See git history for the
# original commented-out text.
# ==============================================================================
