# MINIX 1 x86_64 experiment: boot a serial-reporting Multiboot2 stub with GRUB.
# This does not yet port the legacy MINIX kernel, MM, FS, or userland.

CC = gcc
LD = ld
QEMU = qemu-system-x86_64
GRUB_MKSTANDALONE = grub-mkstandalone
OVMF_CODE = /usr/share/ovmf/OVMF.fd

BUILD_DIR = build/x86_64
KERNEL_ELF = $(BUILD_DIR)/minix.elf
FAT_DIR = $(BUILD_DIR)/efi
GRUB_EFI = $(FAT_DIR)/EFI/BOOT/BOOTX64.EFI

CFLAGS = -m32 -ffreestanding -fno-pie -fno-pic -fno-stack-protector \
         -fno-builtin -O2 -Wall -Wextra
LDFLAGS = -m elf_i386 -nostdlib -z max-page-size=0x1000 -T kernel/x86_64/linker.ld

# QEMU_FLAGS = -m 256M -drive file=fat:rw:$(FAT_DIR),format=raw -bios $(OVMF_CODE) -display gtk -no-reboot
QEMU_FLAGS = -m 256M \
    -drive file=fat:rw:$(FAT_DIR),format=raw \
    -bios $(OVMF_CODE) \
    -display gtk \
    -serial stdio \
    -no-reboot

.PHONY: all clean run debug kernel image help

all: image

help:
	@echo "MINIX 1 VGA terminal demo (32-bit kernel on x86_64 QEMU)"
	@echo "  all, image  Build UEFI GRUB files for QEMU's FAT drive"
	@echo "  kernel      Build the freestanding kernel ELF"
	@echo "  run         Open the VGA terminal in QEMU"
	@echo "  debug       Boot paused with GDB stub on port 1234"
	@echo "  clean       Remove generated build files"
	@echo "Requires grub-mkstandalone, QEMU FAT drive support and OVMF."

# Legacy: kernel: ; $(MAKE) -C kernel
# That sub-make builds mpx88.s and the original i8088 kernel, not this ELF.
kernel: $(KERNEL_ELF)

$(BUILD_DIR):
	mkdir -p $@

$(BUILD_DIR)/boot.o: kernel/x86_64/boot.S | $(BUILD_DIR)
	$(CC) -m32 -ffreestanding -fno-pie -c $< -o $@

$(BUILD_DIR)/kernel.o: kernel/x86_64/kernel.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(KERNEL_ELF): $(BUILD_DIR)/boot.o $(BUILD_DIR)/kernel.o kernel/x86_64/linker.ld
	$(LD) $(LDFLAGS) -o $@ $(BUILD_DIR)/boot.o $(BUILD_DIR)/kernel.o

# Legacy: tools/bootblok.s and its boot image are 16-bit BIOS-specific.
image: $(GRUB_EFI) $(FAT_DIR)/boot/minix.elf

# $(BUILD_DIR)/grub.cfg: | $(BUILD_DIR)
# 	printf '%s\n' 'set timeout=0' 'set default=0' 'insmod all_video' 'insmod gfxterm' 'set gfxmode=auto' 'set gfxpayload=keep' 'terminal_output gfxterm' 'menuentry "MINIX 1 terminal" {' '  multiboot2 /boot/minix.elf' '  boot' '}' > $@
$(BUILD_DIR)/grub.cfg: | $(BUILD_DIR)
	printf '%s\n' \
	    'set timeout=0' \
	    'set default=0' \
	    'insmod all_video' \
	    'insmod multiboot2' \
	    'terminal_output console' \
	    'menuentry "MINIX 1 terminal" {' \
	    '  multiboot2 /boot/minix.elf' \
	    '  boot' \
	    '}' > $@

$(GRUB_EFI): $(KERNEL_ELF) $(BUILD_DIR)/grub.cfg | $(BUILD_DIR)
	mkdir -p $(FAT_DIR)/EFI/BOOT
	$(GRUB_MKSTANDALONE) -O x86_64-efi -o $@ --modules="part_gpt fat normal multiboot2 search search_fs_file" "boot/grub/grub.cfg=$(BUILD_DIR)/grub.cfg" "boot/minix.elf=$(KERNEL_ELF)"

$(FAT_DIR)/boot/minix.elf: $(KERNEL_ELF)
	mkdir -p $(FAT_DIR)/boot
	cp $< $@

# QEMU opens a VGA window and boots the standalone GRUB EFI loader.
run: image
	$(QEMU) $(QEMU_FLAGS)

debug: image
	$(QEMU) $(QEMU_FLAGS) -s -S

clean:
	rm -rf build

# Legacy targets retained for reference; their original makefiles are 16-bit.
# mm: ; $(MAKE) -C mm
# fs: ; $(MAKE) -C fs
# lib: ; $(MAKE) -C lib
