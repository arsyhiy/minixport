# asmc — tiny assembler for MINIX

This prototype is a deliberately small two-pass assembler.  The goal is
not to replace a real toolchain but to provide an assembler that can be
rebuilt inside MINIX using only the tiny libc (`read`, `write`, `open`,
`close`, `malloc`) that MINIX will eventually expose to user programs.

## What it does

- parses a small, fixed instruction set (subset of AT&T syntax):
  - `mov eax, imm`
  - `xor eax, eax`
  - `push imm` / `pop eax`
  - `inc eax` / `dec eax`
  - `int imm8`
  - `jmp` / `call` / `je` / `jz` / `jne` / `jnz`  — relative, resolved on pass 2
  - `nop` / `ret`
- emits a flat binary: no headers, no relocations, just bytes
- `.org` / `.byte` / `.word` / `.long` / `.text` / `.data` / `.globl` accepted
- comments are `;`
- labels are `name:`

## Why a prototype instead of `as`

The existing `bootblok_i386.s` and `kernel/x86_64/*.S` are already written
for GNU `as` with `--32`.  That is fine on the host, but:

1. `as` is a host tool — you can not run it inside MINIX.
2. The MINIX assembler prototype is designed around a much smaller grammar
   so the C parser stays under 500 lines and can later be ported to the
   MINIX libc without changes.

## Two-pass design

The source is read twice:

| Pass | Job |
|------|-----|
| 1 | collect labels and their offsets; all branch displacements are treated as 0 |
| 2 | emit bytes; resolve labels using the table from pass 1 |

This is the smallest reliable way to handle forward references without
building an intermediate IR or doing backpatching.

## Instruction encoding notes

Only the simplest 32-bit flat encodings are used:

| mnemonic | encoding |
|----------|----------|
| `nop` | `90` |
| `ret` | `c3` |
| `int imm8` | `cd imm8` |
| `mov eax, imm` | `b8 imm32` |
| `xor eax, eax` | `31 c0` |
| `inc eax` / `dec eax` | `40` / `48` |
| `pop eax` | `58` |
| `push imm8` | `6a imm8` |
| `push imm32` | `68 imm32` |
| `jmp` | `e9 rel32` |
| `call` | `e8 rel32` |
| `je`/`jz` | `0f 84 rel32` |
| `jne`/`jnz` | `0f 85 rel32` |

All `rel32` values are calculated as `target - (pc + 4)`.

## Limitations

- The instruction set is deliberately minimal — it is enough for the
  current kernel boot stub and for a future userland libc, but not for
  general assembly.
- The prototype writes a flat binary, not ELF objects.  There is no linker
  because the current MINIX modules are all flat binaries.
- `.text` / `.data` / `.globl` are parsed for compatibility but ignored;
  the flat output is whatever the source emits, in order.
- No relocations — symbols are absolute offsets into the output buffer.
- No memory operands (`[addr]`, `[reg+off]`) yet — this is a next step.

## Porting notes (when MINIX runs user programs)

- replace `stdio` calls with `read`/`write`/`open`/`close`
- replace `malloc`/`realloc` with the MINIX `brk`-based allocator or
  keep the fixed-size `output` buffer (it is only 1 MiB)
- the assembler currently assumes a flat memory model; MINIX is flat too,
  so no change is needed
- `pc` starts at 0 — the flat binary is loaded at whatever address the
  image builder places it (the MINIX M1 builder puts it at `0x10000`)

## Build

```sh
make          # builds ./asmc on the host
make test     # assembles hello.s and verifies size
```

To port later, use a MINIX libc instead of `stdio`/`malloc`/`stdio`.
The assembler source is kept small and self-contained for exactly that
reason.
