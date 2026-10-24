#define BUFSIZ  1024
#define NFILES  20
#define NULL       0
#define EOF     (-1)
#define CMASK   0377

#define READMODE     1
#define WRITEMODE    2
#define UNBUFF       4
#define _EOF         8
#define _ERR        16
#define IOMYBUF     32
#define PERPRINTF   64
#define STRINGS    128

#ifndef FILE

extern struct _io_buf {
    int     _fd;
    int     _count;
    int     _flags;
    char   *_buf;
    char   *_ptr;
}  *_io_table[NFILES];


#endif	/* FILE */

#define FILE struct _io_buf


#define stdin  (_io_table[0])	
#define stdout 	(_io_table[1])
#define stderr 	(_io_table[2])

#define getchar() 		getc(stdin)
#define putchar(c) 		putc(c,stdout)
#define puts(s)			fputs(s,stdout)
#define fgetc(f)		getc(f)
#define fputc(c,f)		putc(c,f)
#define feof(p) 		(((p)->_flags & _EOF) != 0)
#define ferror(p) 		(((p)->_flags & _ERR) != 0)
#define fileno(p) 		((p)->_fd)
#define rewind(f)		fseek(f, 0L, 0)
#define testflag(p,x)		((p)->_flags & (x))

/* If you want a stream to be flushed after each printf use:
 * 
 *	perprintf(stream);
 *
 * If you want to stop with this kind of buffering use:
 *
 *	noperprintf(stream);
 */

#define noperprintf(p)		((p)->_flags &= ~PERPRINTF)
#define perprintf(p)		((p)->_flags |= PERPRINTF)



/* ```c */
/* #ifndef MINIX_STDIO_H */
/* #define MINIX_STDIO_H */
/*  */
/* #include <stddef.h> */
/*  */
/* typedef struct _minix_FILE FILE; */
/*  */
/* #define EOF (-1) */
/* #define BUFSIZ 1024 */
/*  */
/* extern FILE *stdin; */
/* extern FILE *stdout; */
/* extern FILE *stderr; */
/*  */
/* int fclose(FILE *stream); */
/* int fflush(FILE *stream); */
/*  */
/* char *fgets(char *str, int n, FILE *stream); */
/* FILE *fopen(const char *path, const char *mode); */
/* int fprintf(FILE *stream, const char *format, ...); */
/* int fputs(const char *str, FILE *stream); */
/* size_t fread(void *ptr, size_t size, size_t count, FILE *stream); */
/* FILE *freopen(const char *path, const char *mode, FILE *stream); */
/* int fseek(FILE *stream, long offset, int whence); */
/* long ftell(FILE *stream); */
/* size_t fwrite(const void *ptr, size_t size, size_t count, FILE *stream); */
/* int getc(FILE *stream); */
/* int getchar(void); */
/* char *gets(char *str); */
/* void perror(const char *str); */
/* int putc(int ch, FILE *stream); */
/* int putchar(int ch); */
/* int scanf(const char *format, ...); */
/* void setbuf(FILE *stream, char *buffer); */
/* int sprintf(char *str, const char *format, ...); */
/* int ungetc(int ch, FILE *stream); */
/*  */
/* #endif /\* MINIX_STDIO_H *\/ */
/* ``` */
