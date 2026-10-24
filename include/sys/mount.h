#ifndef MINIX_SYS_MOUNT_H
#define MINIX_SYS_MOUNT_H

/* MINIX filesystem mount operations */
int mount(const char *special, const char *name, int read_only);
int umount(const char *name);

#endif /* MINIX_SYS_MOUNT_H */
