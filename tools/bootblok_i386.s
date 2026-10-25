# bootblok_i386.S — BIOS boot sector for the M1 educational image.
#
# Assembled with GNU as to 16-bit real-mode code, linked at 0x7C00 and
# stripped to a raw 512-byte sector with the 0xAA55 signature.  It loads the
# system image (the flat binaries appended right after this sector), switches
# the machine to 32-bit protected mode and enters the kernel — the same role
# bootblok.s played in original MINIX 1, minus the a.out segment patching
# (this image keeps flat binaries instead).
#
# Image layout produced by tools/scripts/build.py:
#   sector 0        this boot block
#   sector 1..K     kernel flat binary   (loaded at 0x10000, entry at base)
#   then            mm, fs, init flat binaries (loaded, not yet executed)
#   then            MINIX v1 rootfs      (stays on "disk", future M2 work)
#
# Boot-sector module table (patched by build.py, read by the kernel):
#   offset 500  u16 kernel_sectors
#   offset 502  u16 mm_off          (sector offset inside the image)
#   offset 504  u16 fs_off
#   offset 506  u16 init_off
#   offset 508  u16 rootfs_off      (= end of the loaded region)
#   offset 510  u16 0xAA55 signature


# ---- tunables --------------------------------------------------------------
    .equ LOAD_SECTORS, 96        # sectors after this boot block to load (48K)
    .equ LOAD_SEG,     0x1000    # modules are loaded at 0x1000:0 = 64 KiB
    .equ STACK_TOP,    0x7C00    # real-mode stack grows down from the loader
    .equ SECT_PER_TRACK, 18      # 1.44M floppy geometry, matches QEMU default
    .equ HEADS,        2

# ---- GDT layout ------------------------------------------------------------
    .equ SEL_CODE32, 0x08        # flat 4 GiB code segment, ring 0
    .equ SEL_DATA32, 0x10        # flat 4 GiB data segment, ring 0


    .code16
    .section .text
    .globl _start

_start:
    cli
    xorw %ax, %ax
    movw %ax, %ds
    movw %ax, %es
    movw %ax, %ss
    movw $STACK_TOP, %sp
    sti

    movw $msg_load, %si
    call puts16

    # Load LOAD_SECTORS consecutive sectors starting at LBA 1 into
    # LOAD_SEG:0 upward.  %di = LBA, %bp = sectors left, ES:BX = destination.
    movw $LOAD_SEG, %ax
    movw %ax, %es
    xorw %bx, %bx
    movw $1, %di
    movw $LOAD_SECTORS, %bp

load_loop:
    # translate LBA in %di -> CHS (sector = LBA%18+1, head = LBA/18 % 2,
    # cylinder = LBA/36) — one sector per BIOS call keeps the code simple.
    movw %di, %ax
    xorw %dx, %dx
    movw $SECT_PER_TRACK, %cx
    divw %cx                    # AX = LBA/18, DX = LBA%18
    incw %dx
    movw %dx, %cx               # CL = sector number (1-based)
    xorw %dx, %dx
    movw $HEADS, %si
    divw %si                    # AX = cylinder, DX = head
    movb %al, %ch               # CH = cylinder low byte (image < 256 cyls)
    movb %dl, %dh               # DH = head
    movw $3, %si                # SI = BIOS retry count

load_retry:
    movw $0x0201, %ax           # AH=02 read, AL=1 sector
    movb $0, %dl                # DL = drive 0 = floppy A:
    int  $0x13
    jnc  load_ok
    decw %si
    jnz  load_retry
    movw $msg_disk, %si
    call puts16
    jmp  halt16

load_ok:
    movw %es, %ax
    addw $0x20, %ax             # advance ES:BX by 512 bytes
    movw %ax, %es
    incw %di                    # next LBA
    decw %bp
    jnz  load_loop

    movw $msg_go, %si
    call puts16

    # switch to 32-bit protected mode; GDT lives in this sector at 0x7Cxx
    cli
    lgdt gdt_ptr
    movl %cr0, %eax
    orl  $1, %eax               # PE=1
    movl %eax, %cr0
    ljmp $SEL_CODE32, $pm_start # far jump flushes prefetch, enters pmode

    .code32
pm_start:
    movw $SEL_DATA32, %ax
    movw %ax, %ds
    movw %ax, %es
    movw %ax, %ss
    movw %ax, %fs
    movw %ax, %gs
    movl $0x90000, %esp         # scratch stack, above the loaded image

    movl $(LOAD_SEG << 4), %eax # kernel flat binary sits at phys 0x10000
    call *%eax                  # entry = first byte of the kernel image

halt32:
    hlt
    jmp halt32


    .code16
# puts16: write NUL-terminated string at DS:SI via BIOS teletype.
puts16:
    lodsb
    test %al, %al
    jz   1f
    movb $0x0e, %ah
    movw $0x0007, %bx
    int  $0x10
    jmp  puts16
1:  ret

halt16:
    hlt
    jmp halt16


msg_load: .ascii "bootblok: loading system image\r\n"
          .byte 0
msg_disk: .ascii "bootblok: disk error\r\n"
          .byte 0
msg_go:   .ascii "bootblok: enter pmode\r\n"
          .byte 0

    .align 8
gdt:
    .quad 0                              # null descriptor
    .quad 0x00CF9A000000FFFF             # code32 base 0 limit 4G
    .quad 0x00CF92000000FFFF             # data32 base 0 limit 4G
gdt_ptr:
    .word gdt_ptr - gdt - 1
    .long gdt

# BIOS requires the sector to end with 0x55AA.  The Makefile pads the
# stripped binary to 512 bytes and asserts the signature itself, and
# build.py writes the module table into bytes 500-509 after concatenating
# the image.
