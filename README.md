# minixport — порт MINIX 1 на i386/QEMU

Оригинальные исходники: https://github.com/gdevic/minix1 (MINIX 1, 1987,
Tanenbaum). Цель — довести архитектуру MINIX (kernel + MM + FS + init +
IPC через сообщения) до работающей системы на 32-битном x86 под QEMU.

## Текущее состояние: M1 milestone

Собирается и загружается реальный BIOS-образ в классическом порядке модулей:

```
sector 0    bootblok        BIOS boot sector (загрузчик, 512 байт)
sector 1+   kernel          32-битное ядро, flat binary, грузится в 0x10000
далее       mm, fs, init    flat-бинарники-заглушки (место для портов)
далее       rootfs          настоящий MINIX v1 filesystem (magic 0x137F)
```

Цепочка запуска:

```
BIOS -> bootblok (16-bit) -> int13h загрузка секторов -> GDT -> pmode32
    -> kernel @0x10000 -> читает таблицу модулей из boot sector
    -> печатает "hello, world" на VGA + COM1
```

В boot sector по адресу 500-509 лежит таблица модулей (5×u16), которую
пишет `tools/scripts/build.py` — тот же приём, что использовал
`tools/build.c`, только без сегментных адресов.

## Сборка и запуск

```sh
make            # собрать build/m1/minix.img
make run        # QEMU с GTK-окном (VGA) + serial.log
make run-serial # QEMU без дисплея, вывод COM1 в stdout
make clean
```

Зависимости: `gcc` (с `-m32`), `binutils`, `python3`, `qemu-system-i386`.
На Debian/Ubuntu: `apt install gcc-multilib binutils python3 qemu-system-x86`.

При запуске `make run-serial` в терминале должно появиться:

```
MINIX M1 image — 32-bit protected mode
======================================
kernel   loaded at 0x00010000, boot block at 0x00007c00
module table (build.py):
  kernel : image sector 1  phys 0x00010000
  mm     : image sector 5  phys 0x00010800
  fs     : image sector 6  phys 0x00010a00
  init   : image sector 7  phys 0x00010c00
  rootfs : image sector 8  phys 0x00001000

hello, world — MINIX M1 milestone reached
```

`run` (с VGA-окном) показывает то же самое на эмулируемом текстовом экране.

## Что где лежит

| путь | что это |
|------|---------|
| `Makefile` | сборка M1-образа (kernel/mm/fs/init/rootfs + bootblok) |
| `kernel/x86_64/` | рабочее 32-битное ядро: `boot.S`, `kernel.c`, линкеры, стабы mm/fs/init |
| `kernel/*.c`, `mpx88.s`, `klib88.s` | оригинальное ядро MINIX 1 (i8088, 16 бит) — источник для порта |
| `mm/`, `fs/` | оригинальные Memory Manager и File System (K&R C, 16 бит) |
| `lib/` | библиотеки; `lib/libinit.a` уже собирает syscall-стабы для i386 |
| `tools/bootblok_i386.s` | новый BIOS-загрузчик (GNU as, 16→32 бита) |
| `tools/bootblok.s` | оригинальный загрузчик MINIX (c86-ассемблер, справочно) |
| `tools/scripts/build.py` | сборщик образа + таблица модулей (аналог `tools/build.c`) |
| `tools/scripts/mkfs.py` | генератор настоящего MINIX v1 FS по proto-файлу (аналог `tools/mkfs.c`) |
| `tools/scripts/fsck.py` | проверка целостности MINIX v1 FS (аналог `tools/fsck.c`) |
| `tools/proto.m1` | proto-файл rootfs для M1 (`/dev/tty0`) |
| `tools/init.c` | переписанный init для i386 (собирается, ждёт ядро с IPC) |
| `tools/INIT_I386.md`, `TOOLS_DEMO.md` | заметки по init и tools |

## Архитектура, которую воспроизводим

MINIX 1 — микроядерная ОС: ядро (драйверы+IPC+планировщик), серверы MM и
FS как привилегированные процессы, пользовательские программы общаются с
ними сообщениями через `sendrec()` → `int` (вектор 32 на i8088; в порте
выбран `int 0x80`, см. `lib/syscall/sendrec_i386.s`).

Дорожная карта:

- **M1 (готово)** — образ с настоящей цепочкой bootblok/kernel/mm/fs/init/rootfs; ядро печатает hello world
- M2 — порт ядра: IDT, таймер, TTY-задача, sys_call (send/rec), proc table под i386
- M3 — порт MM (fork/exec/wait/exit/brk) + загрузка ELF init
- M4 — порт FS: mount rootfs, open/read/write → /dev/tty0, настоящий `init` → `sh`

## Правило портирования

В файлах, которые переписываются, оригинальный код сохраняется комментарием
внизу файла (см. `kernel/x86_64/kernel.c`, `boot.S`, `tools/init.c`) —
так видно, что именно изменилось для i386.
