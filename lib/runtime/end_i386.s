/*
 * end_i386.S — End-of-section symbols.
 *
 * Defines symbols marking the boundaries of the text,
 * data, and BSS sections.
 */

.section .text
.globl endtext
endtext:

.section .data
.globl enddata
.globl _edata
enddata:
_edata:

.section .bss
.globl endbss
.globl _end
endbss:
_end:

.section .note.GNU-stack,"",@progbits
