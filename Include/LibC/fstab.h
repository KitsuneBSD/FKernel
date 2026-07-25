#pragma once

#include <LibC/stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

struct fstab {
    char *fs_spec;          /* block special device name */
    char *fs_file;          /* file system path prefix */
    char *fs_vfstype;       /* type of file system, e.g. ufs, nfs */
    char *fs_mntops;        /* comma separated mount options */
    char *fs_type;          /* rw, ro, sw, or xx */
    int fs_freq;            /* dump frequency, in days */
    int fs_passno;          /* pass number on parallel dump */
};

#define FSTAB_RW    "rw"    /* read/write device */
#define FSTAB_RO    "ro"    /* read-only device */
#define FSTAB_SW    "sw"    /* swap device */
#define FSTAB_XX    "xx"    /* ignore totally */

struct fstab *getfsent(void);
struct fstab *getfsspec(const char *);
struct fstab *getfsfile(const char *);
int setfsent(void);
void endfsent(void);

#ifdef __cplusplus
}
#endif
