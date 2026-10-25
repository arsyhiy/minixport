# Что нужно знать, чтобы дописать порт MINIX

Рабочая карта знаний для портирования MINIX 1 (i8088, 16 бит, K&R C) на
32-битный i386 под QEMU. Сгруппировано по областям: от «что уже решено» к
«что ещё предстоит понять».

---

## 1. Архитектура MINIX 1 — что именно портируем

MINIX 1 = микроядро + серверы-процессы:

```
user procs (init, sh, ls, ...)          ring/user mode, обычные процессы
   |  sendrec(message)                   int SYSVEC (i8088) / int 0x80 (порт)
   v
servers:  MM (pid 0..?) / FS            процессы с высшим приоритетом
   |  sendrec / task calls
   v
kernel:   tasks (SYSTASK, CLOCK, MEM, FLOPPY, WINI, TTY, PRINTER)
          proc[NR_TASKS + NR_PROCS] table, 3 очереди планировщика
```

Ключевые числа (`h/const.h`):
- `NR_TASKS = 8` — задачи ядра с отрицательными номерами (`HARDWARE=-1`,
  `SYSTASK=-2`, `CLOCK=-3`, `MEM=-4`, `FLOPPY=-5`, `WINCHESTER=-6`,
  `TTY=-7`, `PRINTER=-8`, `h/com.h`).
- `NR_PROCS = 16` — слоты процессов; `LOW_USER = 2` — индексы 0,1 это
  MM и FS, с 2 начинаются пользовательские (INIT = первый user).
- `NR_SEGS = 3` — T/D/S (text/data/stack) сегменты на процесс.
- 3 очереди планировщика: `TASK_Q=0`, `SERVER_Q=1`, `USER_Q=2`.

IPC-контракт (`h/com.h`): `SEND=1`, `RECEIVE=2`, `BOTH=3`. Сообщение —
`message` union-структура из `h/type.h` (`mess_1..mess_6`, `m_source`,
`m_type`). Сисколл-номера — `h/callnr.h`. Ответ сервера: `m_type >= 0` =
результат, `< 0` = `-errno`.

---

## 2. Уже принятые решения (не передумывать без причины)

- **ABI сисколлов**: `int 0x80`, `EAX=srcdest`, `EBX=&message`,
  `ECX=op (1/2/3)` — `lib/syscall/sendrec_i386.s`. `send`/`receive`
  отдельных стабов пока нет — только `sendrec`.
- **Формат модулей**: flat binary, не a.out и не ELF-загрузка.
  `tools/init` уже собран как ELF32 — но в образе M1 модули плоские;
  решить окончательно на M3 (ELF-лоадер в MM vs objcopy в flat).
- **Загрузка**: BIOS + собственный `tools/bootblok_i386.s`, НЕ GRUB.
  Ядро грузится в 0x10000, точка входа = байт 0 образа.
- **Таблица модулей**: 5×u16 в boot sector по смещению 500 (аналог патчинга
  `tools/build.c`): kernel_sectors, mm_off, fs_off, init_off, rootfs_off.
- **rootfs**: MINIX v1 FS (1K блоки, superblock magic 0x137F, 32-байтные
  иноды, `zone → block` при `log_zone_size=0` = identity).
- **Компилятор**: gcc `-m32 -ffreestanding -fno-pie -fno-pic`, C11;
  оригинальный код — K&R, требует прототипов (`lib.h` уже приведён).
- **Соглашение**: переписываешь файл → оригинал уходит в блочный
  комментарий внизу файла.

---

## 3. Ядро — что предстоит (самый большой кусок)

### 3.1 Машинно-зависимый слой (замена `mpx88.s` + `klib88.s`)

Что делали оригиналы:
- `save()`/`restart()` — сохранение/восстановление контекста в
  `proc[i].p_reg[]` + `p_pcpsw` (pc, cs, psw). На i386 набор регистров
  другой: eax..edi, eip, eflags, cs/ds/es/ss не нужны (flat) — но НУЖНЫ
  user-sелекторы если будет ring3.
- Векторные стабы: `s_call` (int 32→80), `clock_int`, `tty_int`,
  `disk_int`, `wini_int`, `lpr_int`, `surprise`, `trp`, `divide`.
- `klib88.s`: `phys_copy`, `port_in/port_out`, `lock/unlock`,
  `get_chrome`, `get_byte`, `umap`, `wreboot`, `csv`/`setjmp`.

Что нужно на i386:
- **IDT на 256 векторов** + gate-дескрипторы; ремап 8259 PIC
  (`INT_CTL 0x20`, `INT_CTLMASK 0x21`, `INT2_* 0xA0/0xA1` — порты те же);
  IRQ0 таймер → вектор 0x20 после ремапа, клавиатура IRQ1 → 0x21 и т.д.
  Оригинальные номера векторов (`CLOCK_VECTOR=8` и т.д.) — BIOS-специфичны,
  пересчитать.
- **GDT**: минимум kernel code/data (есть в bootblok); для user-процессов
  понадобятся ring3-селекторы + TSS (для iret в юзермод) — ИЛИ запустить
  всё в ring0 на первых итерациях (проще, не-MINIX-пуризм, но работает).
- **`struct proc` переделать**: `p_reg[11]` → i386-регистры (8 GP + eip +
  eflags + esp, ~11 dwords — удачно совпадает по размеру, но смысл иной);
  `p_pcpsw` → {eip, cs, eflags}; `mem_map` — выкинуть clicks, плоские
  phys_base/len или вообще identity-map на старте.
- **`phys_copy`** → обычный `memcpy` в flat-модели; **`umap`** →
  identity (виртуальный = физический), пока нет paging/ring3.
- **`sys_copy` в SYSTASK** (`system.c`) — копия между адресными
  пространствами; в flat-модели вырождается в memcpy с проверкой.
- **`s_call`** — вход сисколла: сохранить регистры юзера в proc-слот,
  переключиться на k_stack, вызвать `sys_call()` (proc.c), вернуться.

### 3.2 Независимая логика (портируется почти verbatim)

- `kernel/proc.c` — `sys_call`, `mini_send/mini_rec`, `ready/unready`,
  `pick_proc`, очереди. Логика чистая, зависит только от `struct proc`.
- `kernel/main.c` — инициализация proc-таблицы, векторов, `restart()`.
  Убрать `set_vec` i8088-вариант, `get_chrome`, BIOS-пробы CPU.
- `kernel/clock.c` — таймер: PIT 8253 (порт 0x40/0x43), `HZ=60`
  (`h/const.h`), accounting user/sys time, alarm-очередь,
  `SCHED_RATE` — рескедюл раз в N тиков.
- `kernel/system.c` — SYSTASK: sys_fork/newmap/exec/xit/copy/times/sig/
  abort — в flat-модели упрощается, но семантика сообщений та же.
- `kernel/memory.c` — MEM task: /dev/ram, /dev/mem, /dev/kmem, /dev/null.
  /dev/ram = RAM-диск для rootfs (см. §5 — образ уже в памяти с 0x10000+).
- `kernel/tty.c` — самый большой драйвер: консоль, termios-подобные
  флаги, очереди ввода/вывода. Для M3 достаточно: TTY_WRITE → VGA,
  TTY_READ → клавиатурная очередь (PS/2 scancode→ascii есть в старой
  демке `kernel.c`), TTY_IOCTL → минимум.
- `kernel/table.c` — `task[]`-массив в порядке `h/com.h`, EXTERN-данные.
- `kernel/dmp.c` — дамп по F1/F2 (отладка, можно на поздних этапах).
- `kernel/floppy.c`, `wini.c`, `at_wini.c`, `xt_wini.c`, `printer.c` —
  **пропустить**: rootfs уже в памяти (RAM-диск), принтер не нужен.
  Оставить stub-задачи или вычеркнуть из `task[]`, скорректировав
  `NR_TASKS`/номера (осторожно: номера захардкожены в `h/com.h` и
  используются FS/MM!).

### 3.3 Ловушки

- `printf` в ядре — это `printk` (`kernel/const.h`: `#define printf printk`);
  реализация в `kernel/tty.c`/`klib` — вывод через порт VGA.
- `proc[i].p_reg[]` индексы и смещения зашиты в mpx88.s константами
  (`esreg=14`, `SP=22`, `PC=24`...) — при новой структуре пересчитать.
- Сигналы: ядро кладёт `sig_info` на стек юзера; `begsig` в lib.
  На первых итерациях можно без сигналов — но MM `signal.c` их ждёт.
- Старт INIT: kernel запускает proc[LOW_USER] = init с `INIT_SP`,
  пушит pc/psw на его стек (`main.c` комментарий про sp=0x10).
  В flat-модели вместо сегментов — выделенный стек + eip на entry.

---

## 4. MM (mm/*.c) — портируется вторым

- `main.c` — цикл `get_work → call_vec[mm_call] → reply`. Почти verbatim.
- `forkexit.c` — fork: выделить слот, `sys_fork` в ядро, скопировать
  память родителя (в flat-модели — memcpy сегмента данных/стека);
  exit/wait, `sys_xit`.
- `exec.c` — разбор заголовка исполняемого файла. Оригинал — a.out
  (32/48-байтный header, `sep_id` bit). Если остаёмся на flat binary —
  формат придумать самим (мы уже пишем flat: text+data+stack = один blob,
  `data_org` magic 0xDADA в head_i386.s). Если ELF — нужен мини-парсер.
- `break.c` — brk/sbrk: ресайз data-сегмента, `sys_newmap`.
- `signal.c` — kill/signal/alarm/pause: через `sys_sig` в ядро.
- `getset.c` — getpid/getuid/set*id — тривиально.
- `alloc.c` — `mem[]` bitmap свободной памяти в clicks; в flat-модели
  заменить на байтовый аллокатор (или оставить clicks=16B — работает и так).
- `table.c`, `glo.h`, `mproc.h`, `param.h` — таблица mproc[], dispatch.
- `mm/putc.c` — вывод паник MM через ядро.

MM получает вызовы от user-процессов (sendrec → MM_PROC_NR=0).

---

## 5. FS (fs/*.c) — портируется третьим

- `main.c` — тот же цикл get_work/call_vector/reply.
- `cache.c` — пул буферов блоков (NR_BUFS), `get_block/put_block`,
  `alloc_zone/free_zone` (формула `z = b + firstdata - 1` — уже
  воспроизведена в mkfs.py), `rw_block` → сообщение драйверу (у нас MEM
  task / RAM-диск).
- `inode.c` — icache, `get_inode/rw_inode`, ilookup.
- `super.c` — mount/load super, `s_imap/s_zmap` слоты.
- `open.c` — open/creat/close; `filedes.c` — filp-таблица;
  `path.c` — `eat_path` разбор путей; `link.c` — link/unlink/rename;
  `read.c`/`write.c` — read_write общий; `pipe.c`; `stadir.c` — stat;
  `protect.c` — chmod/chown/access; `mount.c`; `time.c`; `misc.c`;
  `device.c` — `dev_open/dev_io` → dmap → сообщения задачам (TTY/MEM).
- `table.c`, `glo.h`, `buf.h`, `file.h`, `fproc.h`, `inode.h`,
  `param.h`, `super.h`, `type.h` (d_inode уже понят — см. mkfs.py).
- `fs/putc.c` — паник-вывод FS.

Ключевая идея rootfs на M3+: образ `rootfs.img` уже лежит в памяти
сразу после модулей (наш bootblok грузит 96 секторов = 48K; для rootfs
нужно грузить больше или читать с «диска»). Два пути:
  a) RAM-диск: bootblok грузит rootfs следом (образ ≤ 1.44M влезает),
     MEM-task отдаёт его как блочное устройство — ближе всего к
     оригинальной схеме MINIX (RAM disk через DISK_IOCTL);
  б) floppy-task + чтение по CHS — ближе к железу, но floppy.c портировать
     под 32 бита + DMA — тяжело. Вариант (a) проще.

FS получает вызовы от user-процессов (sendrec → FS_PROC_NR=1) и сама
шлёт сообщения задачам (MEM/TTY/FLOPPY).

---

## 6. Userspace: lib + init + shell

- `lib/` — уже частично портировано: `libinit.a` содержит
  call/close/errno/message/open/read/sync/write + exec/exit/fork/wait +
  `sendrec_i386.o`; `head_i386.s` — `_start` (грузит `stackpt`, зовёт
  `main`). Для полного userspace добавить: stdio (printf → write),
  string, malloc (brk → MM), signal-стабы (`catchsig`, `begsig`).
- `tools/init.c` — готово: запускает `/etc/rc`, читает `/etc/ttys`,
  форкает login на терминал. Зависит от `execn()` (EXEC без argv).
- Shell: `commands/` полон K&R-программ — для M4 достаточно минимального
  `sh` (или вообще test-программы «hello, world» через `write(1,...)`).
- crtso: `lib/runtime/crtso.s` (оригинал) vs `head_i386.s` (порт) —
  argv/env на стеке по конвенции MM EXEC.

---

## 7. Формат исполняемого файла — решить до M3

Оригинальный MINIX exec: a.out header (32 или 48 байт), поля
text/data/bss, `sep_id` bit (separate I&D). `tools/build.c` и `mm/exec.c`
это знают.

Варианты для порта:
1. **flat blob** (текущий M1): header из пары слов {data_size, bss_size}
   или magic+layout — MM просто копирует весь blob в data-сегмент;
   `head_i386.s::_data_org` уже пишет magic 0xDADA как маркер.
2. **ELF32**: `tools/init` уже ELF — но нужен загрузчик в MM
   (program headers, PT_LOAD) — больше кода, стандартнее.
3. **a.out оригинальный**: вернуть старый формат — минимум нового кода
   в mm/exec.c, но тулчейн не выдаёт a.out нативно (нужен objcopy+патч).

Решение влияет на `mm/exec.c`, `tools/makefile` (линковка init),
`build.py` (какие файлы в образ кладём).

---

## 8. Известные грабли и правила проекта

- **`h/` vs `include/`**: `h/*` — внутренние kernel/server заголовки
  (const/type/com/callnr/error); `include/*` — user ABI
  (unistd/stdlib/sys/*). См. `include/README.md`.
- **Clicks**: 1 click = 16 байт (`CLICK_SHIFT=4`), вся память оригинала в
  clicks. В flat-модели можно сохранить clicks для памятной карты —
  решение точечное, но формула `phys = click << 4` размечена по коду.
- **PUBLIC/PRIVATE/EXTERN** (`h/const.h`): PUBLIC="", PRIVATE=static,
  EXTERN=extern кроме table.c.
- **printf в servers**: `mm/putc.c`, `fs/putc.c` — panic-вывод идёт через
  syscall к ядру/TTY, не напрямую.
- **Сообщения m1..m6**: при добавлении полей в `h/type.h` проверять и
  kernel, и lib (message M — единый глобальный буфер на процесс!).
- **`M` глобальный**: `lib/syscall/message.c` — один message-буфер на
  процесс, не thread-safe (в MINIX нет тредов, ок).
- **`callm3`**: строки ≤14 байт копируются в сообщение, длиннее — через
  указатель (MM/FS тогда делают sys_copy из адресного пространства
  звонящего — в flat-модели trivial, в ring3 нужен umap+SYS_COPY).
- **Числа `MM=0`, `FS=1`** в `include/lib.h` = индексы в proc-таблице
  (`proc_addr(who)`) — не путать с pid.
- **bootblok ≤ 510 байт** кода+данных (510 = 512 − sig 2 байта − таблица
  10 байт... на самом деле таблица пишется поверх, код должен уместиться
  до offset 500).
- **`tools/init` ELF vs flat**: в образе сейчас `init.S`-стаб; когда
  init станет настоящим — либо objcopy ELF→flat, либо ELF-лоадер в MM.
- **Компиляция оригинала**: K&R без прототипов ломает C11-фронтенд —
  паттерн уже применён: обернуть сигнатуры в прототипы, тело не трогать
  (см. `lib/process/*.c`).
- **gitignore**: `build/` целиком; `tools/init`, `lib/**/*.o` — уже
  занесено.

## 9. Что читать в коде первым при непонятках

1. `h/com.h` — номера задач и имена полей сообщений (единая шпаргалка).
2. `h/type.h` + `h/callnr.h` — формат message и номера сисколлов.
3. `kernel/proc.c` — сердце IPC (sys_call, deadlock-check, очереди).
4. `mm/param.h` / `fs/param.h` — маппинг полей сообщений на аргументы
   каждого сисколла (какой m-формат у какого call).
5. `tools/build.c` — как оригинальный образ склеивался и что патчилось.
