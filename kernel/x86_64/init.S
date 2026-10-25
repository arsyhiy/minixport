# init.S — M1 init module stub.
#
# Placeholder for the real init binary (tools/init) inside the boot image.
# For M1 it proves the image table carries all four modules; once MM/FS are
# ported, tools/init takes this slot unchanged.

    .section .text.head, "ax"
    .code32
    .globl _start
    .type _start, @function
_start:
    movl $init_stack_top, %esp
    movl $init_msg, %esi
    call serial_puts
1:  hlt
    jmp 1b
    .size _start, . - _start

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
init_msg: .ascii "init module loaded (stub)\r\n"
        .byte 0

    .section .bss
    .align 16
    .skip 4096
init_stack_top:
