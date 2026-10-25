# Инструменты для портирования MINIX — практический гайд

Всё, что реально помогает на этом проекте: что установлено, зачем нужно,
и типовые команды. Сгруппировано по этапам работы.

---

## 1. Тулчейн сборки (обязательное)

| Инструмент | Зачем в проекте | Команды |
|---|---|---|
| **gcc -m32** | весь C-код ядра/MM/FS/lib — freestanding i386 | `gcc -m32 -ffreestanding -fno-pie -c x.c` |
| **gcc-multilib** | заголовки/режим 32 бита; без него `-m32` не работает | `apt install gcc-multilib` |
| **as / GNU assembler** | `bootblok_i386.s` (16-бит!), `boot.S`, стабы модулей | `as --32 -o out.o in.s` |
| **ld** | линковка flat-бинарей и ELF | `ld -m elf_i386 -nostdlib -T linker.ld --oformat binary` |
| **objcopy** | ELF → raw binary, выдёргивание секций | `objcopy -O binary in.elf out.bin` |
| **objdump** | дизассемблирование — главный инструмент отладки | см. ниже |
| **nm / readelf / size** | символы, секции, адреса в ELF | `nm -n kernel.elf \| grep _start` |
| **python3** | `tools/scripts/{mkfs,fsck,build}.py` | `python3 -I script.py args` |
| **make** | точка входа | `make`, `make run-serial`, `make clean` |

Установка на Debian/Ubuntu (есть готовый скрипт `debian_dependencies.sh`):

```sh
sudo apt install build-essential gcc-multilib binutils python3 qemu-system-x86
# grub-pc-bin grub-common xorriso mtools — нужны только если вернёмся к GRUB-образам
```

## 2. objdump — главные рецепты

```sh
# дизассемблер flat-бинаря ядра как 32-битного кода
objdump -D -b binary -m i386 -M intel build/m1/kernel.bin | less

# проверить, что entry = первый байт образа (критично для bootblok!)
objdump -D -b binary -m i386 -M intel build/m1/kernel.bin | head -15

# то же для boot sector — как 16-битный код по адресу 0x7C00
objdump -D -b binary -m i8086 --adjust-vma=0x7C00 build/m1/bootblok.bin

# секции и адреса в ELF до strip
objdump -h build/m1/obj/kernel.o
readelf -hSWs build/m1/obj/kernel.o   # заголовки, секции, символы, reloc
nm -n build/m1/obj/kernel.o           # символы по адресам
```

Правило: после каждой правки линкера/entry проверять, что **байт 0 бинаря —
это `_start`**, иначе bootblok прыгнет не туда.

## 3. QEMU — запуск и отладка

```sh
# обычный запуск (make run / make run-serial обёртки над этим)
qemu-system-i386 -drive format=raw,file=build/m1/minix.img,if=floppy \
    -serial stdio -display none -monitor none -no-reboot

# GDB-отладка: QEMU встаёт на старте, ждёт gdb на :1234
qemu-system-i386 -drive format=raw,file=build/m1/minix.img,if=floppy -s -S

# лог прерываний/экзепшенов/смены CS — находит triple fault мгновенно
qemu-system-i386 ... -d int,cpu_reset -D qemu.log

# монитор QEMU в отдельном канале (info registers, xp /10i $eip)
qemu-system-i386 ... -monitor stdio -serial file:serial.log
# в мониторе:  info registers | info gdt | info idt | xp /20bx 0x7c00

# дамп памяти гостя из монитора
(qemu) pmemsave 0x10000 0x2000 kernel.dump
```

**`-d int -D qemu.log` — самое важное**: при triple fault/reboot-loop
покажет вектор исключения, CS:EIP и регистры в момент падения.

## 4. GDB — отладка ядра пошагово

```sh
# терминал 1: qemu -s -S
# терминал 2:
gdb -ex 'target remote :1234' \
    -ex 'set architecture i386' \
    -ex 'break *0x10000' \
    -ex 'continue' \
    -ex 'layout asm' -ex 'layout regs'

# типовые команды внутри
(gdb) si / stepi            # шаг по инструкции (ядро — только так)
(gdb) x/20i $eip            # дизассемблить вокруг текущей инструкции
(gdb) x/32bx 0x7c00         # hex-дамп boot sector в памяти гостя
(gdb) info registers        # все регистры
(gdb) break *0x10000        # брейк на entry ядра (flat: первый байт)
```

GDB без символов (наши .bin — flat) работает по адресам; для kernel.c
можно слинковать временно ELF-версию и грузить `symbol-file`.

## 5. Hex/бинарный анализ образов

```sh
xxd -s 500 -l 12 build/m1/minix.img     # таблица модулей в boot sector
xxd -s 510 -l 2  build/m1/minix.img     # должно быть 55 aa
xxd -s 1024 -l 32 build/m1/rootfs.img   # superblock: magic 7f 13 в конце
od -An -tx1 -j510 -N2 build/m1/bootblok.bin   # то же через od (есть в Makefile)
hexdump -C build/m1/minix.img | less    # общий просмотр

# проверить длину модуля = границе следующего
stat -c %s build/m1/kernel.bin
```

## 6. Анализ MINIX v1 filesystem

```sh
python3 -I tools/scripts/fsck.py build/m1/rootfs.img   # наш чекер
python3 -I tools/scripts/mkfs.py out.img tools/proto.m1 # пересоздать

# ручной разбор superblock (offset 1024): ninodes nzones imap zmap firstdata
xxd -s 1024 -l 16 build/m1/rootfs.img
# inode #1 (root dir): inode_offset = 2+imap+zmap = блок 4, inode 0
xxd -s 4096 -l 32 build/m1/rootfs.img
# директория root: data zone из inode, dir-entries по 16 байт
xxd -s $((zone*1024)) -l 128 build/m1/rootfs.img
```

`fs/` сервер ожидает ровно этот layout — ручная проверка через xxd
экономит часы отладки mount.

## 7. clangd / IDE (уже настроено)

- `compile_flags.txt` + `clangd.` — clangd видит `-m32 -ffreestanding`
  и наши `-Iinclude -Ih -Ilib`. Любой clangd-клиент (VSCode, Neovim,
  QtCreator) подхватит автоматически.
- Результат: навигация по `h/com.h`, `message`, proc-структурам работает,
  ложные ошибки «функция не объявлена» для freestanding-кода — нет.
- Помнить: clangd подсвечивает код для **i386**, оригинальные файлы
  под `#ifdef i8088` будут серыми — это правильно.

## 8. Git — инструмент порта, а не только VCS

```sh
# оригинал любого переписанного файла — в истории (см. правило проекта)
git log -p --follow kernel/x86_64/kernel.c
git show HEAD~1:kernel/x86_64/kernel.c | less

# сравнить наш i386-файл с оригинальным i8088-аналогом
git diff HEAD -- lib/process/fork.c      # сами правки
diff -u lib/process/fork.c <(git show HEAD~0:lib/process/fork.c)

# найти, где символ использовался в исходном MINIX
git grep -n "sendrec\|SYSVEC\|p_pcpsw" -- 'kernel/*.c' 'lib/*.c'
git grep -n "I_DIRECTORY" h/ fs/          # быстрый поиск по дереву
```

## 9. Сравнение с оригиналом — самый ценный приём порта

```sh
# side-by-side: наш порт vs оригинал
diff -u tools/bootblok.s <(git show HEAD:tools/bootblok.s)   # оригинал не трогали
diff -u <(git show HEAD~N:file) file                          # наш файл vs до правки

# прогнать оригинальную программу в DOS-эмуляторе/16-битной сборке —
# истина по поведению, когда в коде непонятка (см. WHATTOKNOW.md §9)
```

## 10. Python-калькулятор вместо головной арифметики

```sh
python3 -c "print(hex(0x10000 + (5-1)*512))"     # phys адрес модуля по sector off
python3 -c "print((64*1024 - 4*1024)//1024)"     # свободных зон в rootfs
python3 -c "import struct; print(struct.unpack('<8I',
         open('build/m1/minix.img','rb').read()[500:532]))"  # таблица модулей
```

## 11. Что ещё пригодится позже (по мере M2–M4)

| Этап | Инструмент | Зачем |
|---|---|---|
| M2 (IDT/IRQ) | `qemu -d int`, `info pic` в мониторе | векторы, маски 8259 |
| M2 (клавиатура) | `-k en-us`, scancode-дамп через `info mice`/логи | раскладка PS/2 |
| M3 (exec/fork) | GDB + `x/8xw $esp` | проверка user-стека при старте init |
| M3 (ELF) | `readelf -l tools/init` | program headers для MM-лоадера |
| M4 (FS) | `mtools`, `mtools-аналог` или наш mkfs.py | правка rootfs без загрузки ОС |
| любой | `strace -f` на host-утилитах | когда mkfs/build.py делают не то |
| CI позже | `qemu-system-i386 ... -no-reboot -display none` + timeout | headless-прогон образа как тест |

## 12. Минимальный набор «команды на каждый день»

```sh
make && make run-serial                    # собрать и увидеть вывод
objdump -D -b binary -m i386 -M intel build/m1/kernel.bin | head -20
timeout 8 qemu-system-i386 -drive format=raw,file=build/m1/minix.img,if=floppy \
    -serial stdio -display none -d int -D qemu.log && tail -30 qemu.log
xxd -s 500 -l 12 build/m1/minix.img        # таблица модулей
git status && git diff                    # что изменено до коммита
```

Главное правило отладки этого проекта: **serial-порт — ваш stdout с самого
первого такта**, а `objdump -b binary` — единственная правда о том, что
реально попало в образ.
