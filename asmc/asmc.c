/*
 * asmc - tiny two-pass i386 assembler prototype.
 *
 * The output is a flat binary.  The implementation deliberately uses only
 * a small, well-defined instruction subset so it can later be rebuilt inside
 * MINIX with its own libc.  Host-side for now; do not use host ELF output.
 */
#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_LINE 256
#define MAX_SYMBOLS 256
#define MAX_NAME 64
#define MAX_OUTPUT (1024 * 1024)

typedef struct { char name[MAX_NAME]; unsigned long value; } Symbol;
static Symbol symbols[MAX_SYMBOLS];
static int symbol_count;
static unsigned char output[MAX_OUTPUT];
static unsigned long pc;
static int errors;
static int pass;

static void error_at(int line, const char *message, const char *arg)
{
    fprintf(stderr, "asmc:%d: %s%s%s\n", line, message,
            arg ? " `" : "", arg ? arg : "");
    errors++;
}

static char *trim(char *s)
{
    char *end;
    while (isspace((unsigned char)*s)) s++;
    end = s + strlen(s);
    while (end > s && isspace((unsigned char)end[-1])) --end;
    *end = '\0';
    return s;
}

static void strip_comment(char *s)
{
    char *p = strchr(s, ';');
    if (p) *p = '\0';
}

static int is_name(const char *s)
{
    int i;
    if (!(*s == '_' || isalpha((unsigned char)*s))) return 0;
    for (i = 1; s[i]; i++)
        if (!(s[i] == '_' || isalnum((unsigned char)s[i]))) return 0;
    return 1;
}

static int lookup(const char *name, unsigned long *value)
{
    int i;
    for (i = 0; i < symbol_count; i++) {
        if (!strcmp(symbols[i].name, name)) {
            *value = symbols[i].value;
            return 1;
        }
    }
    return 0;
}

static void define_symbol(const char *name, int line)
{
    unsigned long ignored;
    if (!is_name(name)) { error_at(line, "bad label", name); return; }
    if (lookup(name, &ignored)) { error_at(line, "duplicate label", name); return; }
    if (symbol_count == MAX_SYMBOLS) { error_at(line, "too many labels", NULL); return; }
    strncpy(symbols[symbol_count].name, name, MAX_NAME - 1);
    symbols[symbol_count].name[MAX_NAME - 1] = '\0';
    symbols[symbol_count++].value = pc;
}

static int number(const char *s, unsigned long *value)
{
    char *end;
    int base = 0;
    if ((s[0] == '0' || s[0] == '0') && (s[1] == 'x' || s[1] == 'X')) base = 16;
    else if (s[0] == '$') { s++; base = 16; }
    errno = 0;
    *value = strtoul(s, &end, base);
    return !errno && end != s && *trim(end) == '\0';
}

static int value(const char *s, unsigned long *v, int line)
{
    if (number(s, v)) return 1;
    if (pass == 1) { *v = 0; return 1; }
    if (!lookup(s, v)) { error_at(line, "unknown symbol", s); return 0; }
    return 1;
}

static void emit8(unsigned long v)
{
    if (pc >= MAX_OUTPUT) { errors++; return; }
    if (pass == 2) output[pc] = (unsigned char)v;
    pc++;
}

static void emit32(unsigned long v)
{
    emit8(v); emit8(v >> 8); emit8(v >> 16); emit8(v >> 24);
}

static int split(char *s, char **a, char **b)
{
    char *p = s;
    while (*p && !isspace((unsigned char)*p)) p++;
    if (!*p) { *a = s; *b = NULL; return 1; }
    *p++ = '\0';
    while (*p && (isspace((unsigned char)*p) || *p == ',')) p++;
    *a = s; *b = *p ? p : NULL;
    return 1;
}

static int instruction(char *s, int line)
{
    char *op, *arg;
    unsigned long v, target;
    split(s, &op, &arg);
    if (!strcmp(op, "nop")) emit8(0x90);
    else if (!strcmp(op, "ret")) emit8(0xc3);
    else if (!strcmp(op, "int")) {
        if (!arg || !value(arg, &v, line) || v > 255) { error_at(line, "int needs 0..255", arg); return 0; }
        emit8(0xcd); emit8(v);
    } else if (!strcmp(op, "mov") && arg && !strncmp(arg, "eax,", 4)) {
        if (!value(trim(arg + 4), &v, line)) return 0;
        emit8(0xb8); emit32(v);
    } else if (!strcmp(op, "xor") && arg &&
               (!strcmp(trim(arg), "eax,eax") || !strcmp(trim(arg), "eax, eax"))) {
        emit8(0x31); emit8(0xc0);
    } else if (!strcmp(op, "push") && arg) {
        if (!value(trim(arg), &v, line)) return 0;
        if (v <= 127 || v >= (unsigned long)-128) { emit8(0x6a); emit8(v); }
        else { emit8(0x68); emit32(v); }
    } else if (!strcmp(op, "pop") && arg && !strcmp(trim(arg), "eax")) emit8(0x58);
    else if (!strcmp(op, "inc") && arg && !strcmp(trim(arg), "eax")) emit8(0x40);
    else if (!strcmp(op, "dec") && arg && !strcmp(trim(arg), "eax")) emit8(0x48);
    else if ((!strcmp(op, "jmp") || !strcmp(op, "call") || !strcmp(op, "je") ||
              !strcmp(op, "jz") || !strcmp(op, "jne") || !strcmp(op, "jnz")) && arg) {
        if (pass == 1) target = 0;
        else if (!lookup(trim(arg), &target)) { error_at(line, "unknown branch symbol", arg); return 0; }
        if (!strcmp(op, "jmp")) emit8(0xe9);
        else if (!strcmp(op, "call")) emit8(0xe8);
        else { emit8(0x0f); emit8(!strcmp(op, "je") || !strcmp(op, "jz") ? 0x84 : 0x85); }
        /* Relative displacement is patched during pass two. */
        if (pass == 2) {
            long displacement = (long)target - (long)(pc + 4);
            emit32((unsigned long)displacement);
        } else emit32(0);
    } else { error_at(line, "unknown instruction or operand", s); return 0; }
    return 1;
}

static void assemble_line(char *line, int line_number)
{
    char *s, *colon, *directive, *arg;
    char parsed[MAX_LINE];
    unsigned long v;
    strip_comment(line); s = trim(line);
    if (!*s) return;
    colon = strchr(s, ':');
    if (colon) {
        *colon = '\0';
        if (pass == 1) define_symbol(trim(s), line_number);
        s = trim(colon + 1); if (!*s) return;
    }
    strncpy(parsed, s, sizeof(parsed) - 1);
    parsed[sizeof(parsed) - 1] = '\0';
    split(parsed, &directive, &arg);
    if (!strcmp(directive, ".org") && arg) {
        if (!value(trim(arg), &v, line_number)) return;
        if (v < pc) { error_at(line_number, ".org moves backwards", arg); return; }
        while (pc < v) emit8(0);
    } else if (!strcmp(directive, ".byte") || !strcmp(directive, "db")) {
        if (!arg || !value(trim(arg), &v, line_number) || v > 255) error_at(line_number, "byte needs 0..255", arg); else emit8(v);
    } else if (!strcmp(directive, ".word") || !strcmp(directive, "dw")) {
        if (!arg || !value(trim(arg), &v, line_number) || v > 65535) error_at(line_number, "word needs 0..65535", arg); else { emit8(v); emit8(v >> 8); }
    } else if (!strcmp(directive, ".long") || !strcmp(directive, "dd")) {
        if (!arg || !value(trim(arg), &v, line_number)) error_at(line_number, "long needs a value", arg); else emit32(v);
    } else if (!strcmp(directive, ".text") || !strcmp(directive, ".data") || !strcmp(directive, ".globl")) {
        /* Accepted for source compatibility; flat output has no sections. */
    } else instruction(s, line_number);
}

static int run_pass(const char *path, int which)
{
    char line[MAX_LINE];
    int line_number = 0;
    FILE *in = fopen(path, "r");
    if (!in) { perror(path); return 0; }
    pass = which; pc = 0;
    while (fgets(line, sizeof(line), in)) assemble_line(line, ++line_number);
    fclose(in);
    return 1;
}

int main(int argc, char **argv)
{
    FILE *f;
    if (argc != 3) { fprintf(stderr, "usage: asmc input.s output.bin\n"); return 2; }
    if (!run_pass(argv[1], 1) || errors) return 1;
    errors = 0;
    if (!run_pass(argv[1], 2) || errors) return 1;
    f = fopen(argv[2], "wb");
    if (!f) { perror(argv[2]); return 1; }
    if (fwrite(output, 1, pc, f) != pc) { perror(argv[2]); fclose(f); return 1; }
    fclose(f);
    return 0;
}
