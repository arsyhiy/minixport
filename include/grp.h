struct	group { 
	char	*name;
	char	*passwd;
	int	gid;
};

/* ```c */
/* #ifndef MINIX_GRP_H */
/* #define MINIX_GRP_H */
/*  */
/* #include <sys/types.h> */
/*  */
/* struct group { */
/*     char *gr_name; */
/*     char *gr_passwd; */
/*     gid_t gr_gid; */
/*     char **gr_mem; */
/* }; */
/*  */
/* struct group *getgrent(void); */
/* struct group *getgrgid(gid_t gid); */
/* struct group *getgrnam(const char *name); */
/* void setgrent(void); */
/* void endgrent(void); */
/*  */
/* #endif /\* MINIX_GRP_H *\/ */
/* ``` */
