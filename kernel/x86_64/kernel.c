typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;

#define VGA_WIDTH 80
#define VGA_HEIGHT 25
#define VGA_COLOR 0x0f
#define INPUT_SIZE 128

static volatile u16 *const vga = (volatile u16 *)0xb8000;
static u32 cursor;
static u8 shift_down;
static u8 caps_lock;

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

static void clear_line(u32 row)
{
    for (u32 col = 0; col < VGA_WIDTH; col++)
        vga[row * VGA_WIDTH + col] = ((u16)VGA_COLOR << 8) | ' ';
}

static void clear_screen(void)
{
    for (u32 row = 0; row < VGA_HEIGHT; row++)
        clear_line(row);
    cursor = 0;
}

static void put_char(char ch)
{
    if (ch == '\r')
        return;
    if (ch == '\n') {
        cursor = (cursor / VGA_WIDTH + 1) * VGA_WIDTH;
    } else if (ch == '\b') {
        if (cursor > 0) {
            cursor--;
            vga[cursor] = ((u16)VGA_COLOR << 8) | ' ';
        }
    } else if ((unsigned char)ch >= 32) {
        vga[cursor] = ((u16)VGA_COLOR << 8) | (u8)ch;
        cursor++;
    }

    if (cursor >= VGA_WIDTH * VGA_HEIGHT) {
        for (u32 row = 1; row < VGA_HEIGHT; row++)
            for (u32 col = 0; col < VGA_WIDTH; col++)
                vga[(row - 1) * VGA_WIDTH + col] = vga[row * VGA_WIDTH + col];
        clear_line(VGA_HEIGHT - 1);
        cursor = (VGA_HEIGHT - 1) * VGA_WIDTH;
    }
}

static void write_text(const char *text)
{
    while (*text)
        put_char(*text++);
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
    if (ch >= 'a' && ch <= 'z' && caps_lock)
        ch = shift_down ? (char)(ch - 'a' + 'A') : ch;
    else if (ch >= 'A' && ch <= 'Z' && caps_lock)
        ch = shift_down ? (char)(ch - 'A' + 'a') : ch;
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
    write_text("minix> ");
}

static void execute_command(const char *line)
{
    if (strings_equal(line, "help")) {
        write_text("Commands: help, clear, about, echo TEXT\n");
    } else if (strings_equal(line, "clear")) {
        clear_screen();
    } else if (strings_equal(line, "about")) {
        write_text("MINIX 1 x86_64 terminal demo; legacy OS is not ported.\n");
    } else if (starts_with(line, "echo ")) {
        write_text(line + 5);
        put_char('\n');
    } else if (*line) {
        write_text("Unknown command. Type help.\n");
    }
}

void kernel_main(void)
{
    char line[INPUT_SIZE];
    outb(0x3f9, 0x00);
    outb(0x3fb, 0x80);
    outb(0x3f8, 0x03);
    outb(0x3f9, 0x00);
    outb(0x3fb, 0x03);
    outb(0x3fa, 0xc7);
    outb(0x3fc, 0x0b);
    serial_write("KERNEL_MAIN reached\r\n");
    clear_screen();
    write_text("MINIX 1 x86_64 VGA terminal\nType help for commands.\n\n");
    for (;;) {
        u32 length = 0;
        print_prompt();
        for (;;) {
            char ch = keyboard_getchar();
            if (ch == '\n') {
                put_char('\n');
                line[length] = 0;
                execute_command(line);
                break;
            }
            if (ch == '\b') {
                if (length) {
                    length--;
                    put_char('\b');
                }
                continue;
            }
            if (ch >= 32 && length < INPUT_SIZE - 1) {
                line[length++] = ch;
                put_char(ch);
            }
        }
    }
}
