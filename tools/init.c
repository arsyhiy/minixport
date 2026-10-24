

/*
 * This process is the father (mother) of all MINIX user processes.
 * When MINIX comes up, this is process 2. It executes /etc/rc and
 * then reads /etc/ttys to determine which terminals need a login process.
 */

#include "../h/signal.h"
#include "../include/lib.h"
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

#define PIDSLOTS 10
#define STACKSIZE 256
#define DIGIT 8

static int getline(char *buf);
static void startup(int linenr);

char name[] = "/dev/tty?"; /* Terminal names. */
int pid[PIDSLOTS];         /* PIDs of init's own children. */
int pidct;
extern int errno;

char stack[STACKSIZE];
char *stackpt = &stack[STACKSIZE];

int main(void) {
  char line[10]; /* /etc/ttys lines are supposed to be 3 chars. */
  int tty, k, status, ttynr, ct, i;

  /* Carry out /etc/rc. */
  sync();

  if (fork()) {
    /* Parent just waits. */
    wait(&k);
  } else {
    /* Child executes the shell to do the work. */
    if (open("/etc/rc", 0) < 0)
      exit(-1);

    open("/dev/tty0", 1); /* Standard output. */
    open("/dev/tty0", 1); /* Standard error. */

    execn("/bin/sh");
    exit(-2); /* Should never be reached. */
  }

  /* Read /etc/ttys and fork off login processes. */
  tty = open("/etc/ttys", 0);

  if (tty == 0) {
    while ((ct = getline(line)) > 1) {
      if (ct != 4)
        continue;

      if (line[0] != '1')
        continue;

      ttynr = line[2] - '0';

      if (ttynr < 0 || ttynr >= PIDSLOTS)
        continue;

      startup(ttynr);
    }
  } else {
    tty = open("/dev/tty0", 1);
    write(tty, "Init can't open /etc/ttys\n", 26);

    /* The system cannot be started. */
    for (;;)
      ;
  }

  close(tty);

  /*
   * All children have been forked off. Wait for a process to terminate.
   * If it is one of our login processes, restart it. Otherwise, ignore it.
   */
  for (i = 1; i <= NR_SIGS; i++)
    /* signal(i, SIG_IGN); */

  for (;;) {
    k = wait(&status);
    pidct--;

    /* Find the terminal whose process terminated. */
    for (i = 0; i < PIDSLOTS; i++) {
      if (pid[i] == k)
        startup(i);
    }
  }
}

static void startup(int linenr) {
  /* Fork off a process for the indicated terminal. */
  int k;

  k = fork();

  if (k != 0) {
    /* Parent. */
    pid[linenr] = k;
    pidct++;
    return;
  }

  /* Child. */
  close(0); /* /etc/ttys may be open. */

  name[DIGIT] = '0' + linenr;

  if (open(name, 0) != 0)
    exit(-3); /* Standard input. */

  if (open(name, 1) != 1)
    exit(-3); /* Standard output. */

  if (open(name, 1) != 2)
    exit(-3); /* Standard error. */

  execn("/usr/bin/login");
  execn("/bin/login");
  execn("/bin/sh");     /* Last resort if /usr is not mounted. */
  execn("/usr/bin/sh"); /* Last resort if /usr is not mounted. */

  exit(-3); /* No executable could be started. */
}

static int getline(char *buf) {
  /* Read a line from standard input. */
  char *p = buf;

  for (;;) {
    if (read(0, p, 1) <= 0)
      return 0;

    if (*p == '\n') {
      p++;
      *p = '\0';
      return (int)(p - buf);
    }

    p++;
  }
}

/* /\* This process is the father (mother) of all MINIX user processes.  When */
/*  * MINIX comes up, this is process 2.  It executes the /etc/rc shell file and
 */
/*  * then reads the /ect/ttys file to find out which terminals need a login */
/*  * process. */
/*  *\/ */
/*  */
/* #include "../h/signal.h" */
/*  */
/* #define PIDSLOTS          10 */
/* #define STACKSIZE        256 */
/* #define DIGIT              8 */
/*  */ /* char name[] = {"/dev/tty?"};	/\* terminal names *\/ */
/* int pid[PIDSLOTS];		/\* pids of init's own children *\/ */
/* int pidct; */
/* extern int errno; */
/*  */
/* char stack[STACKSIZE]; */
/* char *stackpt = &stack[STACKSIZE]; */
/*  */
/* main() */
/* { */
/*   char line[10];		/\* /etc/ttys lines are supposed to be 3 chars
 * *\/ */
/*   int rc, tty, k, status, ttynr, ct, i; */
/*  */
/*   /\* Carry out /etc/rc. *\/ */
/*   sync();			/\* force buffers out onto RAM disk *\/ */
/*   if (fork()) { */
/* 	/\* Parent just waits. *\/ */
/* 	wait(&k); */
/*   } else { */
/* 	/\* Child exec's the shell to do the work. *\/ */
/* 	if (open("/etc/rc", 0) < 0) exit(-1); */
/* 	open("/dev/tty0", 1);	/\* std output *\/ */
/* 	open("/dev/tty0", 1);	/\* std error *\/ */
/* 	execn("/bin/sh"); */
/* 	exit(-2);		/\* impossible *\/ */
/*   } */
/*  */
/*   /\* Read the /etc/ttys file and fork off login processes. *\/ */
/*   if ( (tty = open("/etc/ttys", 0)) == 0) { */
/* 	/\* Process /etc/ttys file. *\/ */
/* 	while ( (ct = getline(line)) > 1) { */
/* 		if (ct != 4) continue; */
/* 		if (line[0] != '1') continue; */
/* 		ttynr = line[2] - '0'; */
/* 		if (ttynr < 0 || ttynr >= PIDSLOTS) continue; */
/* 		startup(ttynr); */
/* 	} */
/*   } else { */
/* 	tty = open("/dev/tty0", 1); */
/* 	write(tty, "Init can't open /etc/ttys\n", 26); */
/* 	while (1) ;		/\* just hang -- system cannot be started *\/ */
/*   } */
/*   close(tty); */
/*  */
/*   /\* All the children have been forked off.  Wait for someone to terminate.
 */
/*    * Note that it might be a child, in which case a new login process must be
 */
/*    * spawned off, or it might be somebody's orphan, in which case ignore it.
 */
/*    * First ignore all signals. */
/*    *\/ */
/*   for (i = 1; i <= NR_SIGS; i++) signal(i, SIG_IGN); */
/*  */
/*   while (1) { */
/* 	k = wait(&status); */
/* 	pidct--; */
/*  */
/* 	/\* Search to see which line terminated. *\/ */
/* 	for (i = 0; i < PIDSLOTS; i++) */
/* 		if (pid[i] == k) startup(i); */
/*   } */
/* } */
/*  */
/*  */
/* startup(linenr) */
/* int linenr; */
/* { */
/* /\* Fork off a process for the indicated line. *\/ */
/*  */
/*   int k; */
/*  */
/*   if ( (k = fork()) != 0) { */
/* 	/\* Parent *\/ */
/* 	pid[linenr] = k; */
/* 	pidct++; */
/*   } else { */
/* 	/\* Child *\/ */
/* 	close(0);		/\* /etc/ttys may be open *\/ */
/* 	name[DIGIT] = '0' + linenr; */
/* 	if (open(name, 0) != 0) exit(-3);	/\* standard input *\/ */
/* 	if (open(name, 1) != 1) exit(-3);	/\* standard output *\/ */
/* 	if (open(name, 1) != 2) exit(-3);	/\* standard error *\/ */
/* 	execn("/usr/bin/login"); */
/* 	execn("/bin/login"); */
/* 	execn("/bin/sh");	/\* last resort, if mount of /usr failed *\/ */
/* 	execn("/usr/bin/sh");	/\* last resort, if mount of /usr failed *\/ */
/* 	return;			/\* impossible *\/ */
/*   } */
/* } */
/*  */
/*  */
/*  */
/*  */
/*  */
/* int getline(buf) */
/* char *buf; */
/* { */
/* /\* Read a line from standard input. *\/ */
/*  */
/*   char *p; */
/*  */
/*   p = buf; */
/*   while (1) { */
/* 	if (read(0, p, 1) <= 0) return(0); */
/* 	if (*p == '\n') { */
/* 		p++; */
/* 		*p = 0; */
/* 		return(p - buf);	/\* count of chars read *\/ */
/* 	} else { */
/* 		p++; */
/* 	} */
/*   } */
/* } */
