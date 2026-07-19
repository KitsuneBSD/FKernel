#pragma once

#include <LibC/stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* open() flags */
#define O_RDONLY   0x0000  /* open for reading only */
#define O_WRONLY   0x0001  /* open for writing only */
#define O_RDWR     0x0002  /* open for reading and writing */
#define O_ACCMODE  0x0003  /* mask for above modes */

#define O_CREAT    0x0040  /* create if nonexistent */
#define O_EXCL     0x0080  /* error if already exists */
#define O_NOCTTY   0x0100  /* don't assign controlling terminal */
#define O_TRUNC    0x0200  /* truncate to zero length */
#define O_APPEND   0x0400  /* set append mode */
#define O_NONBLOCK 0x0800  /* no delay */
#define O_DSYNC    0x1000  /* synchronize data */
#define O_DIRECT   0x4000  /* direct I/O */
#define O_LARGEFILE 0x8000 /* allow large file opens */
#define O_DIRECTORY 0x10000 /* must be a directory */
#define O_NOFOLLOW  0x20000 /* don't follow symlinks */
#define O_CLOEXEC   0x80000 /* set FD_CLOEXEC */
#define O_SYNC      0x101000 /* synchronize writes */
#define O_PATH      0x200000 /* path only */

/* access modes for access() */
#define F_OK 0  /* test for existence */
#define X_OK 1  /* test for execute permission */
#define W_OK 2  /* test for write permission */
#define R_OK 4  /* test for read permission */

/* fcntl() commands */
#define F_DUPFD    0   /* duplicate file descriptor */
#define F_GETFD    1   /* get file descriptor flags */
#define F_SETFD    2   /* set file descriptor flags */
#define F_GETFL    3   /* get file status flags */
#define F_SETFL    4   /* set file status flags */
#define F_GETLK    5   /* get record locking information */
#define F_SETLK    6   /* set record locking information */
#define F_SETLKW   7   /* set record locking information; wait if blocked */

/* file descriptor flags */
#define FD_CLOEXEC 1   /* close-on-exec flag */

int open(const char *path, int flags, ...);
int close(int fd);
int fcntl(int fd, int cmd, ...);

#ifdef __cplusplus
}
#endif
