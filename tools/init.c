/*
 * init.c — MINIX 1 init process for the i386 user ABI.
 *
 * init is started as process 2.  It runs /etc/rc, reads /etc/ttys, and
 * keeps one login process alive for every enabled terminal.  The program
 * deliberately uses only the small init library; this keeps its image
 * suitable for the original MINIX image builder and for an i386 QEMU boot.
 */

#include "../h/signal.h"
#include "../include/lib.h"
#include "../include/stdlib.h"
#include "../include/sys/wait.h"
#include "../include/unistd.h"

#define PIDSLOTS 10
#define STACKSIZE 256
#define DIGIT 8

static int read_line(char *buf, int size);
static void startup(int linenr);
static void fatal(const char *message);

static char name[] = "/dev/tty?";
static int child_pid[PIDSLOTS];
static int child_count;

/* head_i386.s uses this symbol to install init's initial stack. */
char stack[STACKSIZE];
char *stackpt = &stack[STACKSIZE];

int main(void)
{
    char line[10];
    int tty, status, ttynr, count, i, child;

    sync();

    /* Run the system startup script before enabling login terminals. */
    if (fork() == 0) {
        if (open("/etc/rc", 0) < 0)
            exit(-1);

        open("/dev/tty0", 1);
        open("/dev/tty0", 1);
        execn("/bin/sh");
        exit(-2);
    }
    wait(&status);

    tty = open("/etc/ttys", 0);
    if (tty != 0)
        fatal("Init can't open /etc/ttys\n");

    while ((count = read_line(line, sizeof(line))) > 1) {
        /* An enabled entry has the historical form: "1 0\n". */
        if (count != 4 || line[0] != '1')
            continue;

        ttynr = line[2] - '0';
        if (ttynr >= 0 && ttynr < PIDSLOTS)
            startup(ttynr);
    }
    close(tty);

    /* Reap both login children and unrelated orphaned children. */
    for (;;) {
        child = wait(&status);
        if (child < 0)
            continue;

        if (child_count > 0)
            child_count--;
        for (i = 0; i < PIDSLOTS; i++) {
            if (child_pid[i] == child) {
                startup(i);
                break;
            }
        }
    }
}

static void startup(int linenr)
{
    int child;

    child = fork();
    if (child != 0) {
        child_pid[linenr] = child;
        child_count++;
        return;
    }

    close(0);
    name[DIGIT] = '0' + linenr;

    if (open(name, 0) != 0)
        exit(-3);
    if (open(name, 1) != 1)
        exit(-3);
    if (open(name, 1) != 2)
        exit(-3);

    execn("/usr/bin/login");
    execn("/bin/login");
    execn("/bin/sh");
    execn("/usr/bin/sh");
    exit(-3);
}

static int read_line(char *buf, int size)
{
    char *p = buf;
    char *end = buf + size - 1;

    while (p < end) {
        if (read(0, p, 1) <= 0)
            return 0;
        if (*p == '\n') {
            p++;
            *p = '\0';
            return (int)(p - buf);
        }
        p++;
    }

    /* Consume the rest of an overlong line, then reject it.  Returning a
     * count larger than 4 keeps the caller reading /etc/ttys, like the
     * historical getline(), instead of treating overflow as end of file. */
    while (read(0, p - 1, 1) > 0 && *(p - 1) != '\n')
        ;
    return size;
}

static void fatal(const char *message)
{
    int fd = open("/dev/tty0", 1);
    if (fd >= 0)
        write(fd, (char *)message, len(message) - 1);

    for (;;)
        ;
}

/*
 * Historical MINIX 1 implementation, retained for comparison.
 *
 * The original used K&R declarations, named the helper getline(), relied on
 * an unbounded ten-byte buffer, and left signal handling in the caller.  The
 * i386 version above keeps the same process and descriptor protocol while
 * using prototypes and bounding the /etc/ttys input.
 *
 * main() {
 *   char line[10];
 *   int tty, k, status, ttynr, ct, i;
 *   sync();
 *   if (fork()) wait(&k);
 *   else {
 *     if (open("/etc/rc", 0) < 0) exit(-1);
 *     open("/dev/tty0", 1); open("/dev/tty0", 1);
 *     execn("/bin/sh"); exit(-2);
 *   }
 *   if ((tty = open("/etc/ttys", 0)) == 0) {
 *     while ((ct = getline(line)) > 1) {
 *       if (ct != 4 || line[0] != '1') continue;
 *       ttynr = line[2] - '0';
 *       if (ttynr < 0 || ttynr >= PIDSLOTS) continue;
 *       startup(ttynr);
 *     }
 *   } else {
 *     tty = open("/dev/tty0", 1);
 *     write(tty, "Init can't open /etc/ttys\n", 26);
 *     while (1) ;
 *   }
 *   close(tty);
 *   while (1) {
 *     k = wait(&status); pidct--;
 *     for (i = 0; i < PIDSLOTS; i++)
 *       if (pid[i] == k) startup(i);
 *   }
 * }
 *
 * startup(linenr) int linenr; {
 *   int k = fork();
 *   if (k != 0) { pid[linenr] = k; pidct++; }
 *   else {
 *     close(0); name[DIGIT] = '0' + linenr;
 *     if (open(name, 0) != 0) exit(-3);
 *     if (open(name, 1) != 1) exit(-3);
 *     if (open(name, 1) != 2) exit(-3);
 *     execn("/usr/bin/login"); execn("/bin/login");
 *     execn("/bin/sh"); execn("/usr/bin/sh");
 *   }
 * }
 */
