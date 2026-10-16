// #define VGA_WIDTH 80
// #define VGA_HEIGHT 25
// #define VGA_COLOR 0x0f
#define INPUT_SIZE 128


void vga_clear(void);
void vga_put_char(char ch);
void vga_write(const char *str);


typedef unsigned char  u8;
typedef unsigned short u16;
typedef unsigned int   u32;

#define INPUT_SIZE 128

static u8 shift_down = 0;
static u8 caps_lock = 0;

// // static volatile u16 *const vga = (volatile u16 *)0xb8000;
// static u32 cursor;
// static u8 shift_down;
// static u8 caps_lock;

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

// static void clear_line(u32 row)
// {
//     for (u32 col = 0; col < VGA_WIDTH; col++)
//         vga[row * VGA_WIDTH + col] = ((u16)VGA_COLOR << 8) | ' ';
// }

// static void clear_screen(void)
// {
//     for (u32 row = 0; row < VGA_HEIGHT; row++)
//         clear_line(row);
//     cursor = 0;
// }

// static void put_char(char ch)
// {
//     if (ch == '\r')
//         return;
//     if (ch == '\n') {
//         cursor = (cursor / VGA_WIDTH + 1) * VGA_WIDTH;
//     } else if (ch == '\b') {
//         if (cursor > 0) {
//             cursor--;
//             vga[cursor] = ((u16)VGA_COLOR << 8) | ' ';
//         }
//     } else if ((unsigned char)ch >= 32) {
//         vga[cursor] = ((u16)VGA_COLOR << 8) | (u8)ch;
//         cursor++;
//     }
//
//     if (cursor >= VGA_WIDTH * VGA_HEIGHT) {
//         for (u32 row = 1; row < VGA_HEIGHT; row++)
//             for (u32 col = 0; col < VGA_WIDTH; col++)
//                 vga[(row - 1) * VGA_WIDTH + col] = vga[row * VGA_WIDTH + col];
//         clear_line(VGA_HEIGHT - 1);
//         cursor = (VGA_HEIGHT - 1) * VGA_WIDTH;
//     }
// }

// static void write_text(const char *text)
// {
//     while (*text)
//         put_char(*text++);
// }

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

// static void print_prompt(void)
// {
//     write_text("minix> ");
// }
static void print_prompt(void)
{
    vga_write("minix> ");
}


static void execute_command(const char *line)
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





typedef unsigned char  u8;
typedef unsigned short u16;
typedef unsigned int   u32;

#define VGA_WIDTH  80
#define VGA_HEIGHT 25
#define VGA_SIZE   (VGA_WIDTH * VGA_HEIGHT)

#define VGA_COLOR  0x0F
#define VGA_ENTRY(ch, color) \
    ((u16)(u8)(ch) | ((u16)(color) << 8))

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

void vga_clear(void)
{
    for (u32 row = 0; row < VGA_HEIGHT; row++)
        vga_clear_line(row);

    cursor = 0;
}

void vga_put_char(char ch)
{
    if (ch == '\r') {
        cursor -= cursor % VGA_WIDTH;
        return;
    }

    if (ch == '\n') {
        cursor = (cursor / VGA_WIDTH + 1) * VGA_WIDTH;
    } else if (ch == '\b') {
        if (cursor > 0) {
            cursor--;
            vga[cursor] = VGA_ENTRY(' ', VGA_COLOR);
        }
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
}

void vga_write(const char *str)
{
    if (!str)
        return;

    while (*str)
        vga_put_char(*str++);
}

// void kernel_main(void)
// {
//     char line[INPUT_SIZE];
//     outb(0x3f9, 0x00);
//     outb(0x3fb, 0x80);
//     outb(0x3f8, 0x03);
//     outb(0x3f9, 0x00);
//     outb(0x3fb, 0x03);
//     outb(0x3fa, 0xc7);
//     outb(0x3fc, 0x0b);
//     serial_write("KERNEL_MAIN reached\r\n");
//
//     volatile u16 *test_vga = (volatile u16 *)0xB8000;
//     test_vga[0] = 0x0F4B;  // Белая K
//     test_vga[1] = 0x0F45;  // Белая E
//     test_vga[2] = 0x0F52;  // Белая R
//                            //
//     clear_screen();
//     write_text("MINIX 1 x86_64 VGA terminal\nType help for commands.\n\n");
//     for (;;) {
//         u32 length = 0;
//         print_prompt();
//         for (;;) {
//             char ch = keyboard_getchar();
//             if (ch == '\n') {
//                 put_char('\n');
//                 line[length] = 0;
//                 execute_command(line);
//                 break;
//             }
//             if (ch == '\b') {
//                 if (length) {
//                     length--;
//                     put_char('\b');
//                 }
//                 continue;
//             }
//             if (ch >= 32 && length < INPUT_SIZE - 1) {
//                 line[length++] = ch;
//                 put_char(ch);
//             }
//         }
//     }
// }
//



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
