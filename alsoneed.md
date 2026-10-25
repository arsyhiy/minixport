# Что ещё нужно для жизни внутри ОС: тулчейн и среда программиста

В гостевой системе уже есть `mined` (`commands/mined1.c` + `mined2.c`) —
полноценный экранный редактор. Но программировать внутри MINIX пока
нельзя: нет ни ассемблера, ни компилятора C, которые работали бы *внутри*
гостя и генерировали код под *наш* ABI (32-бит flat, `int 0x80`).

Всё собирается снаружи: `gcc -m32` на хосте → flat binary → `mkfs.py` →
образ. Этот документ — дорожная карта, как довести систему до
self-hosting, и что из среды нужно по пути.

---

## 1. Почему нельзя просто взять оригинальный компилятор MINIX

`commands/cc.c` в дереве — это **только драйвер**. Он вызывает проходы:

```
PP   = /lib/cpp        препроцессор
CEM  = /lib/cem        фронтенд C → EM bytecode
OPT  = /usr/lib/opt    оптимизатор EM
CG   = /usr/lib/cg     кодогенератор EM → asm
ASLD = /usr/bin/asld   ассемблер + линкер в одном
```

Это **Amsterdam Compiler Kit (ACK)** — писался в Vrije Universiteit,
распространялся отдельно, в дереве исходников MINIX 1 его проходов нет
(в `commands/c86/` остался только `make.bat`).

Даже если достать оригинал, он **не подходит напрямую**: cem/cg/asld
генерируют 16-битный сегментный код i8086 и формат a.out MINIX. Наш порт —
32-битный flat protected mode с другим ABI сисколлов. Поэтому:

- оригинальный ACK — это **референс для изучения** (как устроен реальный
  ретро-тулчейн), но не заготовка для порта;
- современный ACK (см. ссылки) умеет target `pc86`/MINIX — опять же
  16 бит;
- вывод: компилятор под нашу систему придётся **писать или адаптировать
  самим**. Хорошая новость — для flat-модели это намного проще, чем
  звучит.

---

## 2. Что должно работать в ОС ДО любого компилятора

Компилятор — обычная user-программа. Порядок зависимостей (по факту это
и есть план M2–M4):

```
IDT + IRQ + TTY-драйвер        ← M2: mined и sh невозможны без ввода/вывода
   └─ FS-сервер + RAM-диск     ← читать .c, писать .o
        └─ MM: fork/exec/wait/brk ← запускать cc-проходы, malloc
             └─ libc: stdio + env ← argv/envp, printf→write
                  └─ toolchain: as → cc → make
```

Конкретный чеклист:

### 2.1 Ядро / драйверы
- [ ] **TTY-драйвер**: чтение PS/2-клавиатуры (scancode→ascii, код в старой
      демке `kernel.c` уже есть) + вывод на VGA.
- [ ] **Raw/cooked режим через ioctl**: `mined` дёргает
      `ioctl(fd, TIOCGETP/TIOCSETP/TIOCGETC/TIOCSETC, struct sgttyb)`
      (`mined1.c:1086–1123`). Нужен `sgtty`-уровень — сырые символы без
      эха, минимум line discipline.
- [ ] **Сигналы**: `mined` живёт на `SIGQUIT` (`^\` — единственный
      оставленный сигнал). Шеллу нужен `SIGINT` на `^C`. Значит, `sys_sig`
      в ядре + `mm/signal.c` + `catchsig/begsig` в lib.
- [ ] **`sys_copy`/umap**: MM/FS читают память звонящего (строки аргументов
      >14 байт идут по указателю — `callm3`).

### 2.2 Userspace-инфраструктура
- [ ] **Решить формат исполняемого файла** — WHATTOKNOW.md §7. Тулчейн
      внутри ОС должен генерить именно его. Прагматика: простейший
      вариант — flat blob `{magic, text_size, data_size, bss_size}` +
      код; тогда линкер вообще не нужен для однофайловых программ.
- [ ] **libc достаточный для жизни**: stdio (`fopen/fread/fwrite/printf`
      → наши read/write), `malloc` (brk→MM), `string.h`, `getenv`
      (env при exec — нужен `mined`у для `$TERM`).
- [ ] **termcap**: `mined1.c:1965` зовёт `tgetent(entry, getenv("TERM"))`.
      Нужна маленькая реализация `tgetent/tgetstr/tgoto` (~300 строк,
      можно портировать из MINIX 2/3 или NetBSD libterm) + файл
      `/etc/termcap` в rootfs с записью для нашей VGA-консоли.
- [ ] **shell**: `commands/sh1..sh5.c` — портируется после MM. Без шелла
      `cc`-драйверу нечем запускать проходы (он сам форкает через
      `execv`, но набирать команды надо где-то).
- [ ] **`/etc/{rc,ttys,passwd,termcap}`**, `/tmp` для промежуточных
      файлов cc (`cc.c` пишет t-файлы в TMPDIR), `/dev/{tty,null,mem}`.
- [ ] **Перенос файлов хост↔гость**: до своего компилятора жить внутри =
      править исходники в `mined`, собирать на хосте, класть в rootfs.
      Сейчас путь один — пересобрать `rootfs.img` через `mkfs.py`.
      Стоит добавить host-утилиту «положить файл в готовый rootfs»
      (или `put` в `fsck.py`) — иначе каждый чих = пересборка образа.

---

## 3. Ассемблер внутри ОС — первый реальный шаг

Ассемблер делать проще компилятора, и он нужен в любом случае (наши
`head_i386.s`, `sendrec_i386.s`, `crtso.s` — asm).

### Вариант A: свой мини-`as` (рекомендуется)
Двухпроходной, подмножество синтаксиса GAS/AT&T (как у нас в `.s`-файлах):

- pass 1: метки + размер инструкций → таблица символов;
- pass 2: кодирование;
- инструкции для старта: `mov/push/pop/add/sub/cmp/jmp/jcc/call/ret/int/
  lea/xor/inc/dec` + директивы `.text/.data/.globl/.byte/.word/.long/
  .ascii/.space` — покрывает crtso, sendrec, setjmp;
- вывод: сразу наш flat-exec (без .o и линкера!) или простейший
  relocatable формат — см. §5;
- объём: ~1.5–2K строк C. Проблема номер один — кодировка инструкций
  x86 (ModRM/SIB), держать под рукой таблицу опкодов (ссылки ниже).

### Вариант B: исторический `as86`/`ld86` (dev86, Bruce Evans)
Тот самый ассемблер, которым собирали бут-сектора Linux 0.x и ELKS.
Аутентичен эпохе, исходники маленькие — но заточен под 8086/16 бит,
для нашего i386-кода подходит плохо. Референс, не решение.

### Вариант C: вообще без as
Если компилятор сразу эмитит бинарь (как `c4` эмитит VM-код) — текстовый
ассемблер не нужен на старте. Экономит один этап, но asm-файлы
(lib/syscall/*, crtso) придётся переписать на C или держать хост-сборку.

---

## 4. Компилятор C внутри ОС — лестница из трёх ступеней

Не надо сразу целиться в полный C. Проверенный путь:

### Ступень 1 — Small-C-подмножество (~2–4K строк)
Подмножество: `int/char/unsigned`, указатели, массивы, `if/while/for/
return`, вызовы функций, глобалы, строки, `#define/#include` (примитивный
cpp). Без: `struct/union` (или только flat struct), `float`, `long long`,
прототипов — K&R-стиль, что ложится на наш код идеально.

- эмитит **текст asm нашего мини-as** (из §3) или сразу flat binary;
- этого хватает, чтобы собрать «hello, world», утилиты уровня `cat`,
  и — главное — **самого себя** (self-hosting маленького cc реален);
- модель: **SubC** (Nils Holm) — self-hosting компилятор подмножества C,
  эмитит i386 AT&T-asm, сопровождается книгой «Practical Compiler
  Construction» со всеми листингами. **Лучший образец для копирования.**

### Ступень 2 — полный C89/K&R (~10K строк)
- **chibicc** (Rui Ueyama): 10K строк, каждый коммит = одна фича, по нему
  написан compilerbook. Эмитит x86-64 — retarget на i386 ощутим, но
  архитектура (tokenize→parse→codegen, рекурсивный спуск) копируется
  один в один.
- **lcc**: i386-target есть из коробки, есть книга «A Retargetable C
  Compiler» — объясняет устройство до строки.
- **Nora Sandler — «Writing a C Compiler»**: пошаговый блог-курс, эмитит
  x86 AT&T-asm — можно писать свой cc по ней и сразу под наш формат.

### Ступень 3 — «взрослый» тулчейн
- **TinyCC (tcc)**: C99 + встроенные ассемблер и линкер, есть i386-target.
  Порт в гостя = самый тяжёлый (нужен POSIX-ish libc: `mmap` не нужен,
  но `stdio/malloc/setjmp` полноценные), зато получаем настоящий cc, коим
  собираются все `commands/*` и, теоретически, ядро.
- Альтернатива «наоборот»: **swieros** — учебная ОС, где C-компилятор
  `c4` self-hosting из коробки; посмотреть, как устроен минимальный
  замкнутый цикл «ОС ↔ свой cc».

### Обходной вариант — интерпретатор раньше компилятора
Если хочется *программировать внутри* раньше, чем дойдёт cc: порт
маленького интерпретатора (tinylisp/SectorLISP — 1K строк, tiny BASIC)
в разы дешевле и требует только read/write/printf. Не заменяет тулчейн,
но «жить» можно уже на M3.

---

## 5. Линкер и формат объектников

- Для старта линкер **не нужен**: однофайловая компиляция → сразу
  исполняемый blob. Так MINIX и не делала .o для простых вещей.
- Когда понадобится: свой relocatable формат — `{header, text, data,
  symtab, relocs}` — ld сводится к «склей секции, пропатчить адреса по
  таблице релокаций». ~500–800 строк. Под flat-модель без сегментов это
  тривиально по сравнению с ELF.
- `commands/ar.c` уже в дереве — архиватор `libinit.a` внутри гостя
  реален. `commands/libpack.c`/`libupack.c` — тоже.
- Не забыть: `mm/exec.c` должен уметь грузить то, что эмитит наш cc —
  решение из WHATTOKNOW.md §7 фиксирует контракт с обеих сторон.

---

## 6. Полная среда «жить как программист» — итоговый список

| Компонент | Статус | Комментарий |
|---|---|---|
| Редактор | `mined` в дереве | нужен raw-tty (sgtty) + termcap + SIGQUIT |
| Shell | `sh1–sh5.c` в дереве | порт после MM |
| C compiler | **отсутствует** | путь §4: mini-cc → subc/chibicc → tcc |
| Assembler | **отсутствует** | мини-as подмножества GAS, §3A |
| Linker | **отсутствует** | откладывается; свой формат, §5 |
| make | `commands/make.c` | порт, K&R |
| ar | `commands/ar.c` | порт |
| cpp | **отсутствует** | сначала встроить в cc; потом взять любой маленький (напр. из SubC/ACK) |
| libc | `lib/` частично | нужен stdio + malloc + env |
| termcap db | **отсутствует** | `/etc/termcap`, запись под VGA-консоль |
| утилиты | `commands/` полон | ls/cat/cp/mv/grep/sort/wc/tr/sed-аналоги есть |
| diff/patch | **нет в дереве** | пригодится: diff написать несложно |
| отладчик | **отсутствует** | реальность = printf; позже — GDB stub через COM1 (OSDev-рецепт) |
| login/getty | `login.c/passwd/su` есть | для однопользовательской разработки — опционально |
| перенос файлов | mkfs.py снаружи | хочется «положить файл в rootfs» без пересборки + выгрузку наружу (serial/xmodem-подобное) |

Правило зависимости одно: **сначала M2 (TTY) и M3 (MM+FS+exec)** —
всё остальное в таблице это просто user-программы.

---

## 7. Ссылки для изучения

### Как устроены маленькие компиляторы (главное)
- **Nils Holm — SubC + «Practical Compiler Construction»** —
  https://www.t3x.org/subc/ — self-hosting компилятор подмножества C,
  эмитит i386-asm; исходники + книга с полными листингами. **Стартовать
  отсюда.**
- **Rui Ueyama — chibicc** — https://github.com/rui314/chibicc —
  коммит-за-коммитом от hello-world до полного C; компаньон-курс
  https://www.sigbus.info/compilerbook (яп., но код читается без него).
- **Nora Sandler — «Writing a C Compiler»** —
  https://norasandler.com/2017/11/29/Write-a-Compiler.html — пошаговая
  серия, эмитит x86 AT&T-asm.
- **Robert Swier — c4 / swieros** — https://github.com/rswier/c4,
  https://github.com/rswier/swieros — C-компилятор в 4 функциях и ОС,
  которая его self-host'ит; эталон минимализма.
- **Jack Crenshaw — «Let's Build a Compiler»** —
  https://compilers.iecc.com/crenshaw/ — классика 1988 года, рекурсивный
  спуск с нуля (на Pascal, идеи переносятся).
- **Austin Henley — «Let's Make a Teeny Tiny Compiler»** —
  https://austinhenley.com/blog/teenytinycompiler1.html — самый мягкий
  вход в тему.
- **Niklaus Wirth — «Compiler Construction»** —
  https://people.inf.ethz.ch/wirth/CompilerConstruction/ — короткая
  книга классика, весь конвейер компилятора минималистично.
- **Fraser & Hanson — «A Retargetable C Compiler»** + lcc —
  https://github.com/drh/lcc — книга+код, есть i386-target.
- **«Crafting Interpreters» (Nystrom)** — https://craftinginterpreters.com —
  лексинг/парсинг/AST, если база по компиляторам нулевая.
- **Cooper & Torczon — «Engineering a Compiler»** — серьёзный учебник,
  когда захочется оптимизаций и регистровой аллокации.

### Ретро-тулчейны — как это делали тогда
- **Amsterdam Compiler Kit** — https://github.com/davidgiven/ack —
  тот самый cem/opt/cg/asld, современная живая версия; смотреть, как
  устроены проходы и EM-промежуточное представление.
- **dev86 (as86/ld86/bcc)** — https://github.com/lkundrak/dev86 —
  16-битный тулчейн эпохи (Linux 0.x, ELKS); эталон «каким был ассемблер».
- **ELKS** — https://github.com/ghaerr/elks — живая 16-битная UNIX-подобная
  ОС со своим тулчейном; смотреть интеграцию as/ld/libc.
- **TinyCC** — https://bellard.org/tcc/, https://repo.or.cz/tinycc.git —
  конечная цель ступени 3.
- **M2-Planet / stage0** — https://github.com/oriansj/M2-Planet,
  https://bootstrappable.org — bootstrap от hex-ручного кода до
  self-hosting cc; теория «собрать компилятор из ничего».

### Кодировка инструкций x86 (для мини-as и codegen)
- **Intel SDM, Vol. 2** — instruction set reference, Appendix A (opcode
  map): https://www.intel.com/content/www/us/en/developer/articles/technical/intel-sdm.html
- **felixcloutier.com/x86** — https://www.felixcloutier.com/x86/ —
  удобная HTML-версия SDM Vol.2 с таблицами кодировок.
- **GNU as manual** — https://sourceware.org/binutils/docs/as/ —
  синтаксис AT&T, который разумно копировать в мини-as.

### ОС-специфика для self-hosting
- **OSDev: Meaty Skeleton** — https://wiki.osdev.org/Meaty_Skeleton —
  эталонный рецепт «собери libc под свою ОС правильно (hosted vs
  freestanding)».
- **OSDev: GCC Cross-Compiler** — https://wiki.osdev.org/GCC_Cross-Compiler —
  зачем кросс-тулчейн обязателен даже если есть локальный gcc.
- **OSDev: GDB Stub** — https://wiki.osdev.org/GDB_stub — отладка ядра/
  программ гостя через COM1 с полноценным gdb на хосте.
- **xv6** — https://pdos.csail.mit.edu/6.828/ — как 32-битный учебный
  Unix решает proc/exec (self-hosting там нет, но userland-контракт
  образцовый).

---

## 8. Предлагаемый порядок работ

1. **M2**: IDT/IRQ/PIT, TTY-драйвер (клавиатура + VGA + sgtty-ioctl),
   сигналы — открывает mined и sh.
2. **M3**: MM (fork/exec/wait/brk), FS + RAM-диск, формат exec-файла,
   crtso+argv/env. Шелл + mined + базовые `commands/*` живут в госте.
3. **Host-side UX**: утилита «записать файл в rootfs.img» + пересборка
   без mkfs-заново; ведение исходников в mined внутри, сборка на хосте.
4. **M5-тулчейн**: мини-as (§3A) → mini-cc Small-C-подмножества (§4.1)
   → self-host mini-cc внутри гостя = точка «живём внутри».
5. Рост: расширять cc (структуры, cpp), затем subc/tcc-путь; ar+make
   внутри; GDB stub по COM1.

Финишная прямая проекта — когда `kernel/`, `mm/`, `fs/` пересобираются
тулчейном, запущенным внутри самой MINIX.
