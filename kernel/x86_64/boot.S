/* boot.S — M1 kernel entry stub.
 *
 * The BIOS boot sector (tools/bootblok_i386.s) loaded this flat binary at
 * physical 0x10000 and enters at offset 0 with:
 *   - 32-bit protected mode already enabled
 *   - flat 4 GiB code/data segments
 *   - no paging, no IDT yet, a scratch stack we should replace
 *
 * This stub puts a private kernel stack in place and calls kernel_main().
 * Interrupts stay off: the IDT arrives with the M2 milestone.
 *
 * --- history ---------------------------------------------------------------
 * The previous version of this file implemented a GRUB Multiboot2 header
 * and expected GRUB to do the load.  The M1 milestone boots the image
 * directly through BIOS like the original MINIX bootblok did, so the
 * multiboot machinery was removed rather than kept dormant.  It is kept in
 * git history (kernel/x86_64/boot.S before this commit) for comparison.
 * Original retained for reference:
 *
 * .set MULTIBOOT2_HEADER_MAGIC, 0xe85250d6
 * .section .multiboot, "a"
 * multiboot_header:
 *     .long MULTIBOOT2_HEADER_MAGIC ... checksum ... flags ...
 * _start:
 *     cli
 *     movl $stack_top, %esp
 *     pushl %ebx
 *     pushl %eax
 *     call kernel_main
 * ---------------------------------------------------------------------------
 */

    .section .text.head, "ax"
    .code32
    .globl _start
    .type _start, @function
_start:
    cli
    movl $stack_top, %esp
    xorl %ebp, %ebp
    call kernel_main
1:  hlt
    jmp 1b
    .size _start, . - _start

    .section .bss
    .align 16
stack_bottom:
    .skip 16384
stack_top:
