/* linker_init.ld — init flat binary for the M1 image. */
OUTPUT_FORMAT(elf32-i386)
ENTRY(_start)
SECTIONS
{
    . = 0x30000;
    .text : { *(.text.head) *(.text*) }
    .rodata : { *(.rodata*) }
    .data : { *(.data*) }
    .bss : { *(COMMON) *(.bss*) }
}
