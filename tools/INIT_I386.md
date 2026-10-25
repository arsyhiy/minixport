# init for i386 — build and QEMU notes

## What changed

`tools/init.c` was rewritten as a C11 freestanding program for the 32-bit
i386 target.  The original MINIX 1 code is preserved in a comment block at
the bottom of the file — scroll down to compare K&R vs. the modern version.
Behaviour is unchanged: run /etc/rc, read /etc/ttys, fork one login per
enabled terminal, and respawn them when they exit.

## Dependencies (what init actually calls)

| init.c call | implementation            |
|-------------|---------------------------|
| sync()      | lib/syscall/sync.c        |
| fork()      | lib/process/fork.c        |
| wait()      | lib/process/wait.c        |
| open()      | lib/syscall/open.c        |
| read()      | lib/syscall/read.c        |
| write()     | lib/syscall/write.c       |
| close()     | lib/syscall/close.c       |
| exit()      | lib/process/exit.c        |
| execn()     | lib/process/exec.c        |
| sendrec()   | lib/syscall/sendrec_i386.s|
| M, errno    | lib/syscall/message.c + errno.c |
| _start      | lib/runtime/head_i386.s   |

These are packed into `lib/libinit.a` by `lib/Makefile` — a deliberately
tiny archive, not a full libc.

## Build

```sh
make -C lib      # builds lib/libinit.a and lib/runtime/head_i386.o
make -C tools    # links tools/init (ELF 32-bit i386, entry _start)
```

The link uses `ld -m elf_i386 -nostdlib --entry=_start`, i.e. there is no
glibc and no host crt0 — `head_i386.o` supplies `_start`, which loads
`stackpt` from init.c and calls `main()`.

## QEMU

`tools/init` is a plain ELF executable, not a bootable image.  To run it in
QEMU you still need the MINIX kernel/MM/FS packed by the image builder
(tools/build.c) into a boot image.  Then boot that image, for example as a
floppy:

```sh
qemu-system-i386 -drive format=raw,file=<image>,if=floppy
```

## Headers

`include/README.md` explains which headers are the user ABI
(unistd.h/stdlib.h/sys/wait.h) and which are internal to the library
(lib.h, h/*).  Include paths used by both Makefiles:
`-I../include -I../h`.
