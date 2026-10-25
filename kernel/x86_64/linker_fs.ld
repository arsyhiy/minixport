/* linker_fs.ld — file-system flat binary for the M1 image. */
OUTPUT_FORMAT(elf32-i386)
ENTRY(_start)
SECTIONS
{
    . = 0x28000;
    .text : { *(.text.head) *(.text*) }
    .rodata : { *(.rodata*) }
    .data : { *(.data*) }
    .bss : { *(COMMON) *(.bss*) }
}
