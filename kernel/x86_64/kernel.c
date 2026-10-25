/*
 * kernel.c — M1 kernel entry.
 *
 * Runs at physical 0x10000 after the BIOS boot sector hands over control in
 * 32-bit protected mode.  For the M1 milestone the kernel does three things:
 *
 *   1. initialize the VGA text console and COM1 serial line
 *   2. read the module table that build.py patched into the boot sector
 *      (bytes 500-509 at 0x7C00) and report where each module landed
 *   3. print the milestone banner and halt
 *
 * The old code below (interactive VGA terminal with keyboard polling and a
 * toy command interpreter) is preserved in a comment for comparison — it
 * was a standalone demo, not MINIX architecture.
 */

typedef unsigned char  u8;
typedef unsigned short u16;
typedef unsigned int   u32;

#define VGA_BASE   ((volatile u16 *)0xB8000)
#define VGA_WIDTH  80
#define VGA_HEIGHT 25
#define VGA_COLOR  0x0F

#define COM1       0x3F8
#define BOOT_SECTOR 0x7C00            /* where BIOS dropped the boot block */
#define MOD_TABLE  (BOOT_SECTOR + 500)/* module table written by build.py */

struct module_table {
    u16 kernel_sectors;
    u16 mm_off;
    u16 fs_off;
    u16 init_off;
    u16 rootfs_off;
};

static inline u8 inb(u16 port)
{
    u8 v;
    __asm__ volatile ("inb %1, %0" : "=a"(v) : "Nd"(port));
    return v;
}

static inline void outb(u16 port, u8 value)
{
    __asm__ volatile ("outb %0, %1" : : "a"(value), "Nd"(port));
}

static void serial_init(void)
{
    outb(COM1 + 1, 0x00);   /* no interrupts yet */
    outb(COM1 + 3, 0x80);   /* DLAB */
    outb(COM1 + 0, 0x03);   /* 38400 baud divisor */
    outb(COM1 + 1, 0x00);
    outb(COM1 + 3, 0x03);   /* 8N1 */
    outb(COM1 + 2, 0xC7);   /* FIFO on */
    outb(COM1 + 4, 0x0B);   /* RTS/DTR */
}

static void serial_putc(char c)
{
    while (!(inb(COM1 + 5) & 0x20))
        ;
    outb(COM1, (u8)c);
}

static void serial_puts(const char *s)
{
    while (*s) {
        if (*s == '\n')
            serial_putc('\r');
        serial_putc(*s++);
    }
}

static u32 vga_cursor;

static void vga_clear(void)
{
    for (u32 i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++)
        VGA_BASE[i] = (u16)' ' | ((u16)VGA_COLOR << 8);
    vga_cursor = 0;
}

static void vga_putc(char c)
{
    if (c == '\n') {
        vga_cursor = (vga_cursor / VGA_WIDTH + 1) * VGA_WIDTH;
    } else if ((u8)c >= 32) {
        VGA_BASE[vga_cursor++] = (u16)(u8)c | ((u16)VGA_COLOR << 8);
    }
    if (vga_cursor >= VGA_WIDTH * VGA_HEIGHT) {
        /* scroll one line */
        for (u32 i = 0; i < (VGA_HEIGHT - 1) * VGA_WIDTH; i++)
            VGA_BASE[i] = VGA_BASE[i + VGA_WIDTH];
        for (u32 i = 0; i < VGA_WIDTH; i++)
            VGA_BASE[(VGA_HEIGHT - 1) * VGA_WIDTH + i] =
                (u16)' ' | ((u16)VGA_COLOR << 8);
        vga_cursor = (VGA_HEIGHT - 1) * VGA_WIDTH;
    }
}

static void vga_puts(const char *s)
{
    while (*s)
        vga_putc(*s++);
}

/* Every kernel message goes to both outputs, like the original printk(). */
static void kprint(const char *s)
{
    vga_puts(s);
    serial_puts(s);
}

static void kprint_dec(u32 v)
{
    char buf[11];
    int i = 10;
    buf[i] = 0;
    if (!v) {
        kprint("0");
        return;
    }
    while (v) {
        buf[--i] = '0' + (v % 10);
        v /= 10;
    }
    kprint(&buf[i]);
}

static void kprint_hex(u32 v)
{
    static const char d[] = "0123456789abcdef";
    kprint("0x");
    for (int i = 7; i >= 0; i--)
        vga_putc(d[(v >> (i * 4)) & 0xF]),
        serial_putc(d[(v >> (i * 4)) & 0xF]);
}

static void report_module(const char *name, u16 sector_off, u32 loaded_phys)
{
    kprint("  ");
    kprint(name);
    kprint(": image sector ");
    kprint_dec(sector_off);
    kprint("  phys ");
    kprint_hex(loaded_phys);
    kprint("\n");
}

void kernel_main(void)
{
    const struct module_table *mt = (const struct module_table *)MOD_TABLE;

    serial_init();
    vga_clear();

    kprint("\n");
    kprint("MINIX M1 image — 32-bit protected mode\n");
    kprint("======================================\n");
    kprint("kernel   loaded at ");
    kprint_hex(0x10000);
    kprint(", boot block at ");
    kprint_hex(BOOT_SECTOR);
    kprint("\n");

    kprint("module table (build.py):\n");
    /* modules were loaded contiguously at 0x10000, starting at image
     * sector 1; physical = 0x10000 + (sector_off - 1) * 512.  rootfs stays
     * on the "disk" — the loader stops before it. */
    report_module("kernel ", 1, 0x10000);
    report_module("mm     ", mt->mm_off,
                  0x10000 + ((u32)mt->mm_off - 1) * 512);
    report_module("fs     ", mt->fs_off,
                  0x10000 + ((u32)mt->fs_off - 1) * 512);
    report_module("init   ", mt->init_off,
                  0x10000 + ((u32)mt->init_off - 1) * 512);
    report_module("rootfs ", mt->rootfs_off,
                  (u32)mt->rootfs_off * 512);

    kprint("\n");
    kprint("hello, world — MINIX M1 milestone reached\n");
    kprint("(kernel only; mm/fs/init are image stubs, rootfs stays on disk)\n");

    for (;;)
        __asm__ volatile ("hlt");
}


/*
 * ===========================================================================
 * Previous kernel.c — standalone VGA terminal demo, kept for comparison.
 * This version was never reached through the MINIX boot chain; it ran under
 * GRUB Multiboot2 and implemented a toy command line by polling the PS/2
 * keyboard.  The M1 kernel above replaces it with the boot-sector module
 * report, and the M2/M3 milestones will replace the stubs with real
 * MINIX-style task/IPC code.
 *
 * ---- original file -----------------------------------------------------
 *
 * #define INPUT_SIZE 128
 * #define VGA_WIDTH  80
 * #define VGA_HEIGHT 25
 * #define VGA_SIZE   (VGA_WIDTH * VGA_HEIGHT)
 * #define VGA_COLOR  0x0F
 * #define VGA_ENTRY(ch, color) ((u16)(u8)(ch) | ((u16)(color) << 8))
 *
 * void vga_clear(void);
 * void vga_put_char(char ch);
 * void vga_write(const char *str);
 *
 * typedef unsigned char  u8;
 * typedef unsigned short u16;
 * typedef unsigned int   u32;
 *
 * static u8 shift_down = 0;
 * static u8 caps_lock = 0;
 *
 * static inline u8 inb(u16 port) { ... }
 * static inline void outb(u16 port, u8 value) { ... }
 *
 * static int strings_equal(const char *left, const char *right) { ... }
 * static int starts_with(const char *text, const char *prefix) { ... }
 *
 * static char scan_to_ascii(u8 scan) { ...scancode table... }
 * static char keyboard_getchar(void) { ...polls 0x64/0x60... }
 * static void serial_write(const char *text) { ... }
 * static void print_prompt(void) { vga_write("minix> "); }
 *
 * static void execute_command(const char *line)
 * {
 *     if (strings_equal(line, "help"))
 *         vga_write("Commands: help, clear, about, echo TEXT\n");
 *     else if (strings_equal(line, "clear"))
 *         vga_clear();
 *     else if (strings_equal(line, "about"))
 *         vga_write("Experimental x86 kernel; legacy OS is not ported.\n");
 *     else if (starts_with(line, "echo ")) {
 *         vga_write(line + 5);
 *         vga_put_char('\n');
 *     } else if (*line)
 *         vga_write("Unknown command. Type help.\n");
 * }
 *
 * void kernel_main(void)
 * {
 *     ...serial init...
 *     vga_clear();
 *     vga_write("MINIX 1 terminal\n");
 *     vga_write("Type 'help' for commands.\n");
 *     print_prompt();
 *     for (;;) {
 *         char ch = keyboard_getchar();
 *         ...line editing, backspace, execute_command...
 *     }
 * }
 *
 * See git history for the complete previous file — it also contained
 * scroll/backspace/caps-lock handling and an alternate vga_put_char().
 * ===========================================================================
 */
