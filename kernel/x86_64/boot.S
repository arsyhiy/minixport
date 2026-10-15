/* GRUB Multiboot2 enters in 32-bit protected mode. */
.set MULTIBOOT2_HEADER_MAGIC, 0xe85250d6
.set MULTIBOOT2_ARCHITECTURE, 0
.set MULTIBOOT2_HEADER_LENGTH, multiboot_header_end - multiboot_header
.set MULTIBOOT2_CHECKSUM, -(MULTIBOOT2_HEADER_MAGIC + MULTIBOOT2_ARCHITECTURE + MULTIBOOT2_HEADER_LENGTH)

.section .multiboot, "a"
.align 8
multiboot_header:
    .long MULTIBOOT2_HEADER_MAGIC
    .long MULTIBOOT2_ARCHITECTURE
    .long MULTIBOOT2_HEADER_LENGTH
    .long MULTIBOOT2_CHECKSUM
    .short 0
    .short 0
    .long 8
multiboot_header_end:

.section .bss, "aw", @nobits
.align 16
stack_bottom:
    .skip 16384
stack_top:

.section .text
.code32
.global _start
.type _start, @function
_start:
    cli
    movl $stack_top, %esp
    pushl %ebx
    pushl %eax
    call kernel_main
1:
    hlt
    jmp 1b
.size _start, . - _start
