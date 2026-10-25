# Как пользоваться проектом

Короткая инструкция по сборке и запуску M1-образа MINIX на i386/QEMU.

## Требования

```sh
sudo apt install gcc-multilib binutils python3 qemu-system-x86
```

`gcc-multilib` обязателен — сборка идёт с флагом `-m32`.

## Сборка

```sh
make            # собирает build/m1/minix.img
```

Что происходит:

1. `tools/bootblok_i386.s` ассемблируется в 512-байтный boot sector
2. `kernel/x86_64/{boot.S,kernel.c}` линкуются в flat binary `kernel.bin`
3. `kernel/x86_64/{mm,fs,init}.S` — в стаб-модули `mm.bin`, `fs.bin`, `init.bin`
4. `tools/scripts/mkfs.py` генерирует `rootfs.img` (MINIX v1 FS) по `tools/proto.m1`
5. `tools/scripts/fsck.py` проверяет файловую систему
6. `tools/scripts/build.py` склеивает всё в `build/m1/minix.img`

## Запуск

### Вариант 1: в терминал (serial)

```sh
make run-serial
```

Вывод пойдёт в stdout прямо здесь. Выход — `Ctrl+C` или `Ctrl+A` потом `X`.

### Вариант 2: с окном VGA

```sh
make run
```

Откроется окно QEMU с текстовым экраном. Serial-дубль пишется в
`build/m1/serial.log`. Закрыть — крестик на окне или `kill` процесса.

### Руками

```sh
qemu-system-i386 -drive format=raw,file=build/m1/minix.img,if=floppy \
    -serial stdio -display none
```

## Что должно появиться

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
(kernel only; mm/fs/init are image stubs, rootfs stays on disk)
```

## Очистка

```sh
make clean      # удаляет build/
```

## Полезные команды

```sh
# пересобрать только загрузчик
make build/m1/bootblok.bin

# только rootfs
make build/m1/rootfs.img

# посмотреть serial-лог прошлого запуска
cat build/m1/serial.log

# посмотреть таблицу модулей в boot sector
xxd -s 500 -l 12 build/m1/minix.img
```

## Если не собирается

| Ошибка | Причина |
|--------|---------|
| `bits/libc-header-start.h: No such file` | нет `gcc-multilib` |
| `qemu-system-i386: command not found` | нет `qemu-system-x86` |
| `bootblok: N bytes > 510` | загрузчик разросся — упростить `bootblok_i386.s` |
| `fsck: bad super magic` | сломался `mkfs.py` — смотри proto-файл |

## Структура образа

```
sector 0    bootblok          BIOS boot sector + таблица модулей (500-509)
sector 1+   kernel.bin        32-битное ядро, flat, вход @0x10000
далее       mm.bin, fs.bin, init.bin   стабы-модули
далее       rootfs.img        MINIX v1 filesystem
```

Дальше — M2: порт настоящего ядра (`kernel/*.c`, `mpx88.s`) с IPC через
`sendrec()`/`int 0x80`, чтобы стабы заменились реальными MM/FS/INIT.
