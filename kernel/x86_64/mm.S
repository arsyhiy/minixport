# mm.S — M1 memory-manager module stub.
#
# Occupies the MM slot in the boot image so build.py always lays the image
# out in the canonical kernel/mm/fs/init order.  For M1 the real memory
# manager (the mm directory, still 16-bit K&R) is not ported; this stub just
# announces itself over COM1 and parks, which keeps the binary tiny and
# verifies the image table end-to-end.

    .section .text.head, "ax"
    .code32
    .globl _start
    .type _start, @function
_start:
    movl $mm_stack_top, %esp
    movl $mm_msg, %esi
    call serial_puts
1:  hlt
    jmp 1b
    .size _start, . - _start

# serial_puts: NUL-terminated string at %esi -> COM1 (0x3F8).
# Port-mapped UART needs no IDT, so stubs can speak before the kernel
# finishes its own console setup.
serial_puts:
    lodsb
    test %al, %al
    jz   1f
    movb %al, %dl
2:  movw $0x3fd, %dx
    inb  %dx, %al
    test $0x20, %al
    jz   2b
    movw $0x3f8, %dx
    movb %dl, %al
    outb %al, %dx
    jmp  serial_puts
1:  ret

    .section .rodata
mm_msg: .ascii "mm module loaded (stub)\r\n"
        .byte 0

    .section .bss
    .align 16
    .skip 4096
mm_stack_top:
