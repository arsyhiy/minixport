/*
 * sendrec_i386.S
 *
 * int sendrec(int srcdest, message *msg);
 *
 * i386, 32-bit protected mode, cdecl ABI.
 *
 * Register convention for our kernel ABI:
 *   EAX = destination/source
 *   EBX = message pointer
 *   ECX = operation (BOTH)
 *   EAX = return value
 */

.section .text
.code32

.globl sendrec
.type sendrec, @function

.equ BOTH, 3
.equ SYSCALL_VECTOR, 0x80

sendrec:
    pushl %ebx

    /* cdecl arguments, accounting for saved EBX */
    movl 8(%esp), %eax
    movl 12(%esp), %ebx
    movl $BOTH, %ecx

    int $SYSCALL_VECTOR

    popl %ebx
    ret

.size sendrec, .-sendrec

.section .note.GNU-stack,"",@progbits
