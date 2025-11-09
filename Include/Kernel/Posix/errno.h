#pragma once

extern int errno;

#define EPERM 1         /* Operation not permitted */
#define ENOENT 2        /* No such file or directory */
#define ESRCH 3         /* No such process */
#define EINTR 4         /* Interrupted system call */
#define EIO 5           /* I/O error */
#define ENXIO 6         /* Device not configured */
#define E2BIG 7         /* Argument list too long */
#define	EBADF		9		/* Bad file descriptor */
#define ENOMEM 12       /* Cannot allocate memory */
#define EACCES 13       /* Permission denied */
#define EFAULT 14       /* Bad address */
#define EBUSY 16        /* Device busy */
#define EEXIST 17       /* File exists */
#define ENODEV 19       /* Operation not supported by device */
#define ENOTDIR 20      /* Not a directory */
#define EISDIR 21       /* Is a directory */
#define EINVAL 22       /* Invalid argument */
#define ENFILE 23       /* Too many open files in system */
#define EMFILE 24       /* Too many open files */
#define ENOTTY 25       /* Inappropriate ioctl for device */
#define EFBIG 27        /* File too large */
#define ENOSPC 28       /* No space left on device */
#define ESPIPE 29       /* Illegal seek */
#define EROFS 30        /* Read-only filesystem */
#define EPIPE 32        /* Broken pipe */
#define EAGAIN 35       /* Resource temporarily unavailable */
#define ENAMETOOLONG 63 /*  File name too long  */
#define ENOTEMPTY 66    /* Directory not empty */
#define ENOSYS 78       /* Function not implemented */
