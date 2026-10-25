#!/usr/bin/env python3
"""Create a tiny BIOS boot sector for the educational image builder.

The sector prints a message and halts.  It intentionally does not claim to
load an ELF kernel: the old MINIX boot protocol needs CHS geometry, segment
layout, and patched fsck addresses that are not yet part of this port.
"""
from pathlib import Path
import struct
import sys

SECTOR_SIZE = 512
MESSAGE = b"MINIX i386 test boot\r\n\0"

# 16-bit real-mode code at 0x7c00:
#   cli; xor ax,ax; mov ds,ax; mov si, message; lodsb; test; int 10h;
#   loop; hlt forever
CODE = bytes([
    0xFA,                   # cli
    0x31, 0xC0,             # xor ax, ax
    0x8E, 0xD8,             # mov ds, ax
    0xBE, 0, 0,             # mov si, message (patched below)
    0xAC,                   # lodsb
    0x84, 0xC0,             # test al, al
    0x74, 0x08,             # jz halt
    0xB4, 0x0E,             # mov ah, 0x0e
    0xB7, 0x00,             # mov bh, 0
    0xB3, 0x07,             # mov bl, 7
    0xCD, 0x10,             # int 0x10
    0xEB, 0xEE,             # jmp message loop
    0xF4,                   # hlt
    0xEB, 0xFD,             # jmp halt
])


def main() -> int:
    output = Path(sys.argv[1]) if len(sys.argv) == 2 else Path("bootblok.bin")
    sector = bytearray(SECTOR_SIZE)
    message_offset = len(CODE)
    code = bytearray(CODE)
    struct.pack_into("<H", code, 6, 0x7C00 + message_offset)
    sector[: len(code)] = code
    sector[message_offset : message_offset + len(MESSAGE)] = MESSAGE
    sector[510:512] = b"\x55\xAA"
    output.write_bytes(sector)
    print(f"wrote {output} ({len(sector)} bytes, boot signature 0xAA55)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
