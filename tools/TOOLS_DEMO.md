# Учебные tools для i386/QEMU

## Что добавлено

В `tools/scripts/` находятся четыре независимых host-скрипта:

- `bootblok.py` — создаёт ровно 512 байт BIOS boot sector, печатает сообщение
  и останавливается. В конце проверяется сигнатура `55 aa`.
- `build.py` — соединяет boot sector и произвольные payload-файлы в образ.
  Каждый payload выравнивается на 512 байт. В конец добавляется простая
  таблица описаний.
- `mkfs.py` — создаёт маленькую демонстрационную файловую систему из JSON
  manifest-файла.
- `fsck.py` — проверяет magic, таблицу файлов, границы и перекрытия записей.

Это **не полноценные утилиты MINIX 1**. Они нужны для проверки цепочки:

```text
Python scripts -> boot sector -> demo image -> validation
```

Оригинальные `bootblok.s`, `build.c`, `mkfs.c`, `fsck.c` и `fsck1.s` не
удалялись и остаются для дальнейшего настоящего порта MINIX.

## Быстрая проверка

Запускать из каталога `tools`:

```sh
make bootblok
make demo-fs
make demo-fsck
make demo-image
```

Ожидаемый результат:

```text
wrote bootblok.bin (512 bytes, boot signature 0xAA55)
OK: demo/demo.fs entries=2 ...
wrote demo/demo.img (... bytes)
```

Или напрямую:

```sh
python3 -I scripts/bootblok.py bootblok.bin
python3 -I scripts/mkfs.py demo/demo.fs demo/files.json
python3 -I scripts/fsck.py demo/demo.fs
python3 -I scripts/build.py demo/demo.img bootblok.bin \
    init init demo.fs demo/demo.fs
```

`-I` запускает Python в изолированном режиме и не позволяет случайному
файлу из текущего каталога подменить модуль Python.

## Проверка boot sector в QEMU

Созданный `bootblok.bin` можно запустить как дискету:

```sh
qemu-system-i386 \
    -drive format=raw,file=bootblok.bin,if=floppy \
    -display gtk -no-reboot
```

Он должен показать `MINIX i386 test boot`, затем остановиться. Это пока не
загружает `tools/init`: загрузчик не знает формата ELF и не загружает kernel.
`demo/demo.img` также предназначен для проверки формата и пока не является
запускаемым MINIX образом.

## Формат demo filesystem

`demo/files.json` имеет вид:

```json
{
  "files": [
    {"name": "hello.txt", "file": "tools/demo/hello.txt"}
  ]
}
```

Заголовок `MXFS001` и таблица записей придуманы для этого учебного скрипта.
Они не совместимы с superblock/inode layout старого `mkfs.c`.

## Следующий настоящий этап

Чтобы получить реальный MINIX-образ, нужно отдельно:

1. Собрать kernel/MM/FS в одном согласованном i386 формате.
2. Выбрать ELF или старый a.out/flat формат и привести `build` к нему.
3. Реализовать загрузку нескольких секторов BIOS в `bootblok`.
4. Согласовать адреса kernel, MM, FS, init и fsck.
5. Только после этого заменить демонстрационные цели `demo-image` и `demo-fsck`
   настоящими `image` и `fsck`.
