// define section
#define INPUT_SIZE 128

#define VGA_WIDTH  80
#define VGA_HEIGHT 25
#define VGA_SIZE   (VGA_WIDTH * VGA_HEIGHT)

#define VGA_COLOR  0x0F
#define VGA_ENTRY(ch, color) \
    ((u16)(u8)(ch) | ((u16)(color) << 8))

// prototype section
void vga_clear(void);
void vga_put_char(char ch);
void vga_write(const char *str);


// typedef section
typedef unsigned char  u8;
typedef unsigned short u16;
typedef unsigned int   u32;

typedef unsigned char  u8;
typedef unsigned short u16;
typedef unsigned int   u32;

// static section
static u8 shift_down = 0;
static u8 caps_lock = 0;


static inline u8 inb(u16 port)
{
    u8 value;
    __asm__ volatile ("inb %1, %0" : "=a"(value) : "Nd"(port));
    return value;
}

static inline void outb(u16 port, u8 value)
{
    __asm__ volatile ("outb %0, %1" : : "a"(value), "Nd"(port));
}


static int strings_equal(const char *left, const char *right)
{
    while (*left && *left == *right) {
        left++;
        right++;
    }
    return *left == *right;
}

static int starts_with(const char *text, const char *prefix)
{
    while (*prefix && *text == *prefix) {
        text++;
        prefix++;
    }
    return *prefix == 0;
}

static char scan_to_ascii(u8 scan)
{
    static const char plain[] = {
        0, 27, '1','2','3','4','5','6','7','8','9','0','-','=', '\b', '\t',
        'q','w','e','r','t','y','u','i','o','p','[',']','\n', 0, 'a','s',
        'd','f','g','h','j','k','l',';','\'', '`', 0, '\\','z','x','c','v',
        'b','n','m',',','.','/', 0, '*', 0, ' '
    };
    static const char shifted[] = {
        0, 27, '!','@','#','$','%','^','&','*','(',')','_','+', '\b', '\t',
        'Q','W','E','R','T','Y','U','I','O','P','{','}','\n', 0, 'A','S',
        'D','F','G','H','J','K','L',':','"','~', 0, '|','Z','X','C','V',
        'B','N','M','<','>','?', 0, '*', 0, ' '
    };
    if (scan >= sizeof(plain))
        return 0;
    char ch = (shift_down ? shifted : plain)[scan];
if (caps_lock && ch >= 'a' && ch <= 'z') {
    if (!shift_down)
        ch = (char)(ch - 'a' + 'A');
} else if (caps_lock && ch >= 'A' && ch <= 'Z') {
    if (shift_down)
        ch = (char)(ch - 'A' + 'a');
}
    return ch;
}

static char keyboard_getchar(void)
{
    for (;;) {
        if (!(inb(0x64) & 1))
            continue;
        u8 scan = inb(0x60);
        if (scan == 0x2a || scan == 0x36) {
            shift_down = 1;
            continue;
        }
        if (scan == 0xaa || scan == 0xb6) {
            shift_down = 0;
            continue;
        }
        if (scan == 0x3a) {
            caps_lock = !caps_lock;
            continue;
        }
        if (scan & 0x80)
            continue;
        char ch = scan_to_ascii(scan);
        if (ch)
            return ch;
    }
}

static void serial_write(const char *text)
{
    while (*text) {
        while (!(inb(0x3fd) & 0x20))
            ;
        outb(0x3f8, (u8)*text++);
    }
}

static void print_prompt(void)
{
    vga_write("minix> ");
}


static void execute_command(const char *line) // переписать всю обработку комманд в терминальную оболочку
{
    if (strings_equal(line, "help")) {
        vga_write("Commands: help, clear, about, echo TEXT\n");
    } else if (strings_equal(line, "clear")) {
        vga_clear();
    } else if (strings_equal(line, "about")) {
        vga_write("Experimental x86 kernel; legacy OS is not ported.\n");
    } else if (starts_with(line, "echo ")) {
        vga_write(line + 5);
        vga_put_char('\n');
    } else if (*line) {
        vga_write("Unknown command. Type help.\n");
    }
}

static volatile u16 *const vga =
    (volatile u16 *)0xB8000;

static u32 cursor = 0;

static void vga_clear_line(u32 row)
{
    if (row >= VGA_HEIGHT)
        return;

    for (u32 col = 0; col < VGA_WIDTH; col++)
        vga[row * VGA_WIDTH + col] =
            VGA_ENTRY(' ', VGA_COLOR);
}

static void vga_scroll(void)
{
    for (u32 row = 1; row < VGA_HEIGHT; row++) {
        for (u32 col = 0; col < VGA_WIDTH; col++) {
            vga[(row - 1) * VGA_WIDTH + col] =
                vga[row * VGA_WIDTH + col];
        }
    }

    vga_clear_line(VGA_HEIGHT - 1);
    cursor = VGA_SIZE - VGA_WIDTH;
}



static void vga_update_cursor(void)
{
    u16 pos = (u16)cursor;

    outb(0x3D4, 0x0F);
    outb(0x3D5, (u8)(pos & 0xFF));

    outb(0x3D4, 0x0E);
    outb(0x3D5, (u8)((pos >> 8) & 0xFF));
}

void vga_clear(void)
{
    for (u32 row = 0; row < VGA_HEIGHT; row++)
        vga_clear_line(row);

    cursor = 0;
    vga_update_cursor();
}
/* void vga_clear(void) */
/* { */
/*     for (u32 row = 0; row < VGA_HEIGHT; row++) */
/*         vga_clear_line(row); */
/*  */
/*     cursor = 0; */
/* } */


/* void vga_put_char(char ch) */
/* { */
/*     if (ch == '\r') { */
/*         cursor -= cursor % VGA_WIDTH; */
/*         return; */
/*     } */
/*  */
/*     if (ch == '\n') { */
/*         cursor = (cursor / VGA_WIDTH + 1) * VGA_WIDTH; */
/*     } else if (ch == '\b') { */
/*         if (cursor > 0) { */
/*             cursor--; */
/*             vga[cursor] = VGA_ENTRY(' ', VGA_COLOR); */
/*         } */
/*         return; */
/*     } else if ((u8)ch >= 32) { */
/*         if (cursor >= VGA_SIZE) */
/*             vga_scroll(); */
/*  */
/*         vga[cursor++] = VGA_ENTRY(ch, VGA_COLOR); */
/*     } else { */
/*         return; */
/*     } */
/*  */
/*     if (cursor >= VGA_SIZE) */
/*         vga_scroll(); */
/* } */

void vga_put_char(char ch)
{
    if (ch == '\r') {
        cursor -= cursor % VGA_WIDTH;
        vga_update_cursor();
        return;
    }

    if (ch == '\n') {
        cursor = (cursor / VGA_WIDTH + 1) * VGA_WIDTH;
    } else if (ch == '\b') {
        if (cursor > 0) {
            cursor--;
            vga[cursor] = VGA_ENTRY(' ', VGA_COLOR);
        }
        vga_update_cursor();
        return;
    } else if ((u8)ch >= 32) {
        if (cursor >= VGA_SIZE)
            vga_scroll();

        vga[cursor++] = VGA_ENTRY(ch, VGA_COLOR);
    } else {
        return;
    }

    if (cursor >= VGA_SIZE)
        vga_scroll();

    vga_update_cursor();
}

void vga_write(const char *str)
{
    if (!str)
        return;

    while (*str)
        vga_put_char(*str++);
}

void kernel_main(void)
{
    char line[INPUT_SIZE];
    u32 pos = 0;

    outb(0x3F9, 0x00);
    outb(0x3FB, 0x80);
    outb(0x3F8, 0x03);
    outb(0x3F9, 0x00);
    outb(0x3FB, 0x03);
    outb(0x3FA, 0xC7);
    outb(0x3FC, 0x0B);

    vga_clear();

    vga_write("MINIX 1 terminal\n");
    vga_write("Type 'help' for commands.\n");
    print_prompt();

    for (;;) {
        char ch = keyboard_getchar();

        if (ch == '\0')
            continue;

        if (ch == '\n') {
            vga_put_char('\n');
            line[pos] = '\0';
            execute_command(line);
            pos = 0;
            print_prompt();
        } else if (ch == '\b') {
            if (pos > 0) {
                pos--;
                vga_put_char('\b');
            }
        } else if (pos < INPUT_SIZE - 1) {
            line[pos++] = ch;
            vga_put_char(ch);
        }
    }
}
