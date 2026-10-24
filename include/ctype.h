extern	char	_ctype_[];

#define	_U	0001
#define	_L	0002
#define	_N	0004
#define	_S	0010
#define _P	0020
#define _C	0040
#define _X	0100

#define	isalpha(c)	((_ctype_+1)[c]&(_U|_L))
#define	isupper(c)	((_ctype_+1)[c]&_U)
#define	islower(c)	((_ctype_+1)[c]&_L)
#define	isdigit(c)	((_ctype_+1)[c]&_N)
#define	isxdigit(c)	((_ctype_+1)[c]&(_N|_X))
#define	isspace(c)	((_ctype_+1)[c]&_S)
#define ispunct(c)	((_ctype_+1)[c]&_P)
#define isalnum(c)	((_ctype_+1)[c]&(_U|_L|_N))
#define isprint(c)	((_ctype_+1)[c]&(_P|_U|_L|_N))
#define iscntrl(c)	((_ctype_+1)[c]&_C)
#define isascii(c)	((unsigned)(c)<=0177)


/* ```c */
/* #ifndef MINIX_CTYPE_H */
/* #define MINIX_CTYPE_H */
/*  */
/* int isalnum(int ch); */
/* int isalpha(int ch); */
/* int isascii(int ch); */
/* int iscntrl(int ch); */
/* int isdigit(int ch); */
/* int isgraph(int ch); */
/* int islower(int ch); */
/* int isprint(int ch); */
/* int ispunct(int ch); */
/* int isspace(int ch); */
/* int isupper(int ch); */
/* int isxdigit(int ch); */
/*  */
/* int toascii(int ch); */
/* int tolower(int ch); */
/* int toupper(int ch); */
/*  */
/* #endif /\* MINIX_CTYPE_H *\/ */
/* ``` */
