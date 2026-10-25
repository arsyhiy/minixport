# fs.S — M1 file-system module stub.
#
# Same role as mm.S: placeholder occupying the FS slot of the image until
# the fs directory sources are ported.  Reports on COM1 and halts.

    .section .text.head, "ax"
    .code32
    .globl _start
    .type _start, @function
_start:
    movl $fs_stack_top, %esp
    movl $fs_msg, %esi
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
fs_msg: .ascii "fs module loaded (stub)\r\n"
        .byte 0

    .section .bss
    .align 16
    .skip 4096
fs_stack_top:
