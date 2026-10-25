.text
.globl _start
_start:
    mov eax, 1
    push 1
    pop eax
    xor eax, eax
    inc eax
    dec eax
    int 0x80
    ret
