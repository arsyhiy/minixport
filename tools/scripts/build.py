#!/usr/bin/env python3
"""Assemble the M1 boot image in MINIX build.c order.

    build.py OUT.IMAGE BOOTSECT KERNEL MM FS INIT ROOTFS

Layout of the result:

    sector 0                    boot sector (512 bytes, 0xAA55 signature)
    sectors 1..                 kernel flat binary
    then                        mm, fs, init flat binaries
    then                        MINIX v1 root file system image

The last six words of the boot sector are a small module table that the
kernel can read at run time (offsets are sectors from the start of the
image, i.e. the same units the bootloader used when loading):

    offset 500  u16 kernel_sectors
    offset 502  u16 mm_off
    offset 504  u16 fs_off
    offset 506  u16 init_off
    offset 508  u16 rootfs_off

The original tools/build.c did the same job for the 16-bit a.out world: it
concatenated kernel/mm/fs/init/fsck and patched segment addresses into the
boot block and kernel data space.  Our flat binaries need only these five
numbers — module roles are positional, like kernel/mm/fs/init/fsck were.
"""
from pathlib import Path
import struct
import sys

SECTOR = 512
TABLE_OFF = 500                          # first free word before signature


def main() -> int:
    if len(sys.argv) != 8:
        print("usage: build.py OUT BOOT KERNEL MM FS INIT ROOTFS",
              file=sys.stderr)
        return 2

    out, boot_p, kern_p, mm_p, fs_p, init_p, rootfs_p = \
        (Path(a) for a in sys.argv[1:8])

    boot = bytearray(boot_p.read_bytes())
    if len(boot) != SECTOR or boot[510:512] != b"\x55\xaa":
        raise SystemExit("build: boot sector must be 512 bytes + 0xAA55")

    image = bytearray(boot)

    def put(path):
        off = len(image) // SECTOR         # sector index where payload starts
        data = path.read_bytes()
        image.extend(data)
        image.extend(bytes((-len(data)) % SECTOR))
        print(f"  {path.name:<12} sector {off:>4}  {len(data):>6} bytes")
        return off, (len(data) + SECTOR - 1) // SECTOR

    k_off, k_sz = put(kern_p)
    mm_off, _ = put(mm_p)
    fs_off, _ = put(fs_p)
    init_off, _ = put(init_p)
    rootfs_off, _ = put(rootfs_p)

    struct.pack_into("<5H", image, TABLE_OFF,
                     k_sz, mm_off, fs_off, init_off, rootfs_off)

    out.parent.mkdir(parents=True, exist_ok=True)
    out.write_bytes(image)
    print(f"build: wrote {out} ({len(image)} bytes, "
          f"{len(image) // SECTOR} sectors)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
