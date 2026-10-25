/* linker.ld — M1 kernel flat binary.
 *
 * The BIOS boot sector loads this image at physical 0x10000 and jumps to
 * its first byte, so the entry stub must sit at offset 0 and all addressing
 * must match the 64 KiB load address.
 */

OUTPUT_FORMAT(elf32-i386)
ENTRY(_start)

SECTIONS
{
    . = 0x10000;
    .text : AT(0x10000) { *(.text.head) *(.text*) }
    .rodata : { *(.rodata*) }
    .data : { *(.data*) }
    .bss : { *(COMMON) *(.bss*) }
}
