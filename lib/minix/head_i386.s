/*
 * head_i386.S — C program entry point.
 *
 * Initializes the stack and transfers control to main().
 * Used by the MINIX user-space runtime.
 */

.section .text
.code32

.globl _start
.globl _exit

.extern main
.extern stackpt

_start:
    movl stackpt, %esp
    call main

.Lhalt:
    jmp .Lhalt

_exit:
    jmp _exit

.section .data
.globl _data_org

_data_org:
    .long 0xDADA
    .long 0, 0, 0, 0, 0, 0, 0

.section .bss
