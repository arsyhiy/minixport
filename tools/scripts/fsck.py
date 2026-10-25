#!/usr/bin/env python3
"""Minimal sanity checker for a MINIX v1 file system image.

Checks the same invariants the real tools/fsck.c verifies first: super block
magic and geometry, the allocation bitmaps, and that every reachable inode's
direct zones lie inside the data area.  This is not a full fsck — it exists
to catch a broken mkfs.py before the image reaches QEMU.

    fsck.py ROOTFS.IMG
"""
import struct
import sys

BLOCK = 1024
INODE_SIZE = 32
INODES_PER_BLOCK = BLOCK // INODE_SIZE
NR_ZONE_NUMS = 9
NR_DZONE_NUM = NR_ZONE_NUMS - 2
SUPER_MAGIC = 0x137F
I_TYPE = 0o170000
I_CHAR_SPECIAL = 0o020000
I_BLOCK_SPECIAL = 0o060000

SUPER = struct.Struct("<HHHHHHlH")
DINODE = struct.Struct("<HhLlbB9H")
DIRENT = struct.Struct("<H14s")


def bitmap_get(bitmap, bit):
    return (bitmap[bit >> 3] >> (bit & 7)) & 1


def main() -> int:
    if len(sys.argv) != 2:
        print("usage: fsck.py ROOTFS.IMG", file=sys.stderr)
        return 2

    img = open(sys.argv[1], "rb").read()
    if len(img) < 2 * BLOCK or len(img) % BLOCK:
        raise SystemExit("fsck: image not a whole number of blocks")

    nblocks = len(img) // BLOCK
    (ninodes, nzones, imap_b, zmap_b, first_data,
     log_zone, max_size, magic) = SUPER.unpack_from(img, BLOCK)

    if magic != SUPER_MAGIC:
        raise SystemExit(f"fsck: bad super magic 0x{magic:04x}")
    if nzones != nblocks:
        print(f"fsck: warning: s_nzones={nzones} but image has {nblocks} blocks")
    if log_zone != 0:
        raise SystemExit("fsck: log_zone_size != 0 unsupported")
    inode_blocks = (ninodes + 1 + INODES_PER_BLOCK - 1) // INODES_PER_BLOCK
    expect = 2 + imap_b + zmap_b + inode_blocks
    if first_data != expect:
        raise SystemExit(f"fsck: firstdatazone {first_data} != computed {expect}")

    imap = img[2 * BLOCK:(2 + imap_b) * BLOCK]
    zmap = img[(2 + imap_b) * BLOCK:(2 + imap_b + zmap_b) * BLOCK]

    errors = 0
    for inum in range(1, ninodes + 1):
        if not bitmap_get(imap, inum):
            continue
        off = (2 + imap_b + zmap_b) * BLOCK \
            + (inum - 1) // INODES_PER_BLOCK * BLOCK \
            + (inum - 1) % INODES_PER_BLOCK * INODE_SIZE
        (mode, uid, size, mtime, gid, nlinks, *zones) = \
            DINODE.unpack_from(img, off)
        kind = mode & I_TYPE
        used = (size + BLOCK - 1) // BLOCK if size else 0
        for i, z in enumerate(zones[:NR_DZONE_NUM]):
            if z == 0:
                if kind in (I_CHAR_SPECIAL, I_BLOCK_SPECIAL) and i == 0:
                    continue
                if i < used:
                    print(f"fsck: inode {inum}: hole in zones at slot {i}")
                    errors += 1
                continue
            if z < first_data or z >= nblocks:
                if kind in (I_CHAR_SPECIAL, I_BLOCK_SPECIAL) and i == 0:
                    continue            # device numbers live in zone[0]
                print(f"fsck: inode {inum}: zone {z} outside data area")
                errors += 1
            elif not bitmap_get(zmap, z - first_data + 1):
                print(f"fsck: inode {inum}: zone {z} not marked allocated")
                errors += 1

    if errors:
        raise SystemExit(f"fsck: {errors} error(s)")
    print(f"fsck: OK — {nblocks} blocks, {ninodes} inodes, "
          f"first data zone {first_data}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
