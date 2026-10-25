/* linker_mm.ld — memory-manager flat binary for the M1 image.
 * Same trick as the kernel: module body loaded at a fixed physical address,
 * entry at offset 0. */
OUTPUT_FORMAT(elf32-i386)
ENTRY(_start)
SECTIONS
{
    . = 0x20000;
    .text : { *(.text.head) *(.text*) }
    .rodata : { *(.rodata*) }
    .data : { *(.data*) }
    .bss : { *(COMMON) *(.bss*) }
}
