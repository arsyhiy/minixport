#!/usr/bin/env python3
"""Create a real MINIX 1 (v1, 16-bit) file system from a proto file.

Python counterpart of tools/mkfs.c: it understands the same prototype file
format and writes the same on-disk layout MINIX expects:

    block 0                 boot block (zeros; build.py overlays bootblok)
    block 1                 super block   (magic 0x137F)
    s_imap_blocks           inode bitmap
    s_zmap_blocks           zone bitmap
    inode blocks            32-byte d_inode records
    data zones              file and directory contents

Zone numbering follows fs/cache.c: inode i_zone[] entries and the zmap both
count zones, where bit b covers zone b + s_firstdatazone - 1.  With a log
zone size of 0 a zone number is simply the absolute block number.

Proto file format (same as tools/mkfs.c accepts; host paths are resolved
relative to the proto file's directory):

    boot                    # first line: ignored (boot block name)
    <blocks> <inodes>       # device size in 1K blocks, inode count
    d--755 2 1              # root directory mode, uid, gid
       name   ---644 2 1  host/path    regular file
       name   c--644 2 1  MAJ MIN      char device (zone[0] = major<<8|minor)
       name   b--644 2 1  MAJ MIN      block device
       subdir d--755 2 1               directory; contents until '$'
    $                       # ends a directory listing (and the file)
"""
from pathlib import Path
import struct
import sys
import time

BLOCK = 1024
INODE_SIZE = 32
INODES_PER_BLOCK = BLOCK // INODE_SIZE       # 32
NAME_SIZE = 14
DIRENT_SIZE = 16                             # u16 inum + 14-byte name
NR_ZONE_NUMS = 9                             # zone slots in a d_inode
NR_DZONE_NUM = NR_ZONE_NUMS - 2              # direct zones (no indirects yet)
SUPER_MAGIC = 0x137F
BITS_PER_MAP_BLOCK = BLOCK * 8

I_REGULAR = 0o100000
I_DIRECTORY = 0o040000
I_CHAR_SPECIAL = 0o020000
I_BLOCK_SPECIAL = 0o060000


def _mode(kind_ch, perm):
    kind = {"d": I_DIRECTORY, "c": I_CHAR_SPECIAL,
            "b": I_BLOCK_SPECIAL, "-": I_REGULAR}[kind_ch]
    return kind | perm


def _mode_tok(tok):
    """'---644' -> ('-', 0o644); 'd--755' -> ('d', 0o755) ..."""
    return tok[0], int(tok[3:6], 8)


class MinixFS:
    """In-memory image of one MINIX v1 file system."""

    def __init__(self, blocks, inodes):
        self.nblocks = blocks
        self.ninodes = inodes
        self.image = bytearray(blocks * BLOCK)
        self.imap = bytearray(BLOCK)         # inode bitmap block
        self.zmap = bytearray(BLOCK)         # zone bitmap block
        self.inodes = [None]                 # inum indexes this list (1..N)

        self.imap_blocks = 1
        self.zmap_blocks = 1
        self.inode_offset = 2 + self.imap_blocks + self.zmap_blocks
        inode_blocks = (inodes + 1 + INODES_PER_BLOCK - 1) // INODES_PER_BLOCK
        self.first_data_zone = self.inode_offset + inode_blocks
        if self.first_data_zone >= blocks:
            raise SystemExit("mkfs: inode table fills the device")

        # super block
        max_size = (NR_DZONE_NUM + 512 + 512 * 512) * BLOCK
        struct.pack_into("<HHHHHHlH", self.image, BLOCK,
                         inodes, blocks, self.imap_blocks,
                         self.zmap_blocks, self.first_data_zone,
                         0, max_size, SUPER_MAGIC)

        # bit 0 of both maps is always marked busy (never a real object)
        self._set_bits(self.imap, 0, 1)
        self._set_bits(self.zmap, 0, 1)
        # mark everything past the device end as busy so alloc stops;
        # usable zmap bits are 1..(nblocks - firstdata), each standing for
        # zone firstdata - 1 + bit (fs/cache.c uses the same formula)
        self.zone_limit = self.nblocks - self.first_data_zone + 1
        self._set_bits(self.imap, inodes + 1,
                       BITS_PER_MAP_BLOCK - inodes - 1)
        self._set_bits(self.zmap, self.zone_limit,
                       BITS_PER_MAP_BLOCK - self.zone_limit)

    @property
    def nzones(self):
        """Number of zones on the device (= s_nzones)."""
        return self.nblocks                  # log_zone_size == 0

    # ---- bitmaps -----------------------------------------------------
    @staticmethod
    def _set_bits(bitmap, first, count):
        for bit in range(first, first + count):
            bitmap[bit >> 3] |= 1 << (bit & 7)

    def _alloc_bit(self, bitmap, limit, hint=1):
        for bit in range(hint, limit):
            if not (bitmap[bit >> 3] >> (bit & 7)) & 1:
                self._set_bits(bitmap, bit, 1)
                return bit
        raise SystemExit("mkfs: device too small (bitmap exhausted)")

    def alloc_zone(self):
        """Return an inode-level zone number z (block z holds the data)."""
        bit = self._alloc_bit(self.zmap, self.zone_limit)
        return self.first_data_zone - 1 + bit

    def alloc_inode(self):
        return self._alloc_bit(self.imap, self.ninodes + 1)

    # ---- inodes ------------------------------------------------------
    def new_inode(self, mode, uid, gid):
        inum = self.alloc_inode()
        inode = {"mode": mode, "uid": uid, "size": 0,
                 "time": int(time.time()) & 0xFFFFFFFF,
                 "gid": gid, "nlinks": 0, "zones": [0] * NR_ZONE_NUMS}
        while len(self.inodes) <= inum:
            self.inodes.append(None)
        self.inodes[inum] = inode
        return inum

    def write_inode(self, inum):
        ino = self.inodes[inum]
        blk = self.inode_offset + (inum - 1) // INODES_PER_BLOCK
        off = blk * BLOCK + (inum - 1) % INODES_PER_BLOCK * INODE_SIZE
        struct.pack_into("<HhLlbB9H", self.image, off,
                         ino["mode"], ino["uid"], ino["size"], ino["time"],
                         ino["gid"], ino["nlinks"], *ino["zones"])

    def add_data(self, inum, data):
        """Store file bytes in direct zones."""
        ino = self.inodes[inum]
        for i in range(NR_DZONE_NUM):
            off = i * BLOCK
            if off >= len(data):
                break
            if i >= NR_DZONE_NUM:
                raise SystemExit("mkfs: file too big for direct zones")
            zone = self.alloc_zone()
            chunk = data[off:off + BLOCK]
            self.image[zone * BLOCK: zone * BLOCK + len(chunk)] = chunk
            ino["zones"][i] = zone
        if len(data) > NR_DZONE_NUM * BLOCK:
            raise SystemExit("mkfs: file exceeds 7 direct zones")
        ino["size"] = len(data)

    # ---- directories -------------------------------------------------
    def enter_dir(self, parent_inum, name, child_inum):
        parent = self.inodes[parent_inum]
        entry = struct.pack("<H14s", child_inum,
                            name.encode("ascii")[:NAME_SIZE]
                                .ljust(NAME_SIZE, b"\0"))
        for zi in range(NR_DZONE_NUM):
            zone = parent["zones"][zi]
            if not zone:
                zone = self.alloc_zone()
                parent["zones"][zi] = zone
            base = zone * BLOCK
            for i in range(BLOCK // DIRENT_SIZE):
                off = base + i * DIRENT_SIZE
                if struct.unpack_from("<H", self.image, off)[0] == 0:
                    self.image[off:off + DIRENT_SIZE] = entry
                    parent["size"] = max(parent["size"], (zi + 1) * BLOCK)
                    return
            continue
        raise SystemExit("mkfs: directory too big for direct zones")

    # ---- final -------------------------------------------------------
    def flush(self):
        self.image[2 * BLOCK: 3 * BLOCK] = self.imap
        self.image[3 * BLOCK: 4 * BLOCK] = self.zmap
        for inum, ino in enumerate(self.inodes):
            if ino is not None:
                self.write_inode(inum)


def _new_dir(fs, parent_inum, uid, gid, perm):
    inum = fs.new_inode(_mode("d", perm), uid, gid)
    fs.inodes[inum]["nlinks"] = 2           # self + "."
    fs.enter_dir(inum, ".", inum)
    fs.enter_dir(inum, "..", parent_inum)
    fs.inodes[parent_inum]["nlinks"] += 1   # parent gains a ".." link
    return inum


def _eat_dir(fs, base, lines, idx, parent_inum):
    while idx < len(lines):
        toks = lines[idx].split()
        idx += 1
        if not toks:
            continue
        if toks[0] == "$":
            return idx
        name, spec = toks[0], toks[1]
        uid, gid = int(toks[2]), int(toks[3])
        kind, perm = _mode_tok(spec)

        if kind == "d":
            inum = _new_dir(fs, parent_inum, uid, gid, perm)
            fs.enter_dir(parent_inum, name, inum)
            idx = _eat_dir(fs, base, lines, idx, inum)
        elif kind in ("c", "b"):
            inum = fs.new_inode(_mode(kind, perm), uid, gid)
            fs.inodes[inum]["nlinks"] = 1
            fs.inodes[inum]["zones"][0] = (int(toks[4]) << 8) | int(toks[5])
            fs.enter_dir(parent_inum, name, inum)
        else:
            inum = fs.new_inode(_mode("-", perm), uid, gid)
            fs.inodes[inum]["nlinks"] = 1
            src = base / toks[4] if len(toks) > 4 else None
            if src is None or not src.is_file():
                print(f"mkfs: warning: cannot open {toks[4] if len(toks) > 4 else name}",
                      file=sys.stderr)
            else:
                fs.add_data(inum, src.read_bytes())
            fs.enter_dir(parent_inum, name, inum)
    return idx


def build(proto_path, out_path):
    proto = Path(proto_path)
    lines = proto.read_text().splitlines()

    # line 0: boot block name (ignored — build.py puts bootblok there)
    blocks, inodes = (int(x) for x in lines[1].split())
    t = lines[2].split()
    root_kind, root_perm = _mode_tok(t[0])
    assert root_kind == "d", "root entry must be a directory"
    root_uid, root_gid = int(t[1]), int(t[2])

    fs = MinixFS(blocks, inodes)
    root = fs.new_inode(_mode("d", root_perm), root_uid, root_gid)
    fs.inodes[root]["nlinks"] = 2
    fs.enter_dir(root, ".", root)
    fs.enter_dir(root, "..", root)

    _eat_dir(fs, proto.parent, lines, 3, root)
    fs.flush()
    Path(out_path).write_bytes(fs.image)
    print(f"mkfs: {out_path} — {blocks} blocks, "
          f"zones used from {fs.first_data_zone}")


def main():
    if len(sys.argv) != 3:
        print("usage: mkfs.py OUT.IMG PROTO", file=sys.stderr)
        return 2
    build(sys.argv[2], sys.argv[1])
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
