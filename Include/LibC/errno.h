#pragma once

#ifdef __cplusplus
extern "C" {
#endif

extern int errno;

#define EPERM    1   /* Operation not permitted */
#define ENOENT   2   /* No such file or directory */
#define ESRCH    3   /* No such process */
#define EINTR    4   /* Interrupted system call */
#define EIO      5   /* I/O error */
#define ENXIO    6   /* No such device or address */
#define E2BIG    7   /* Argument list too long */
#define ENOEXEC  8   /* Exec format error */
#define EBADF    9   /* Bad file descriptor */
#define ECHILD   10  /* No child processes */
#define EAGAIN   11  /* Resource temporarily unavailable */
#define ENOMEM   12  /* Not enough memory */
#define EACCES   13  /* Permission denied */
#define EFAULT   14  /* Bad address */
#define EBUSY    16  /* Device or resource busy */
#define EEXIST   17  /* File exists */
#define EXDEV    18  /* Cross-device link */
#define ENODEV   19  /* No such device */
#define ENOTDIR  20  /* Not a directory */
#define EISDIR   21  /* Is a directory */
#define EINVAL   22  /* Invalid argument */
#define ENFILE   23  /* File table overflow */
#define EMFILE   24  /* Too many open files */
#define ENOTTY   25  /* Not a typewriter */
#define EFBIG    27  /* File too large */
#define ENOSPC   28  /* No space left on device */
#define ESPIPE   29  /* Illegal seek */
#define EROFS    30  /* Read-only file system */
#define EMLINK   31  /* Too many links */
#define EPIPE    32  /* Broken pipe */
#define EDOM     33  /* Math argument out of domain */
#define ERANGE   34  /* Math result not representable */
#define EDEADLK  35  /* Resource deadlock would occur */
#define ENAMETOOLONG 36 /* File name too long */
#define ENOLCK   37  /* No record locks available */
#define ENOSYS   38  /* Function not implemented */
#define ENOTEMPTY 39 /* Directory not empty */
#define ELOOP    40  /* Too many symbolic links */
#define ENOMSG   42  /* No message of desired type */
#define EIDRM    43  /* Identifier removed */
#define ENOSTR   60  /* Device not a stream */
#define ENODATA  61  /* No data available */
#define ETIME    62  /* Timer expired */
#define ENOSR    63  /* Out of streams resources */
#define EREMOTE  66  /* Object is remote */
#define ENOLINK  67  /* Link has been severed */
#define EPROTO   71  /* Protocol error */
#define EMULTIHOP 72 /* Multihop attempted */
#define EBADMSG  74  /* Bad message */
#define EOVERFLOW 75 /* Value too large for defined data type */
#define EILSEQ   84  /* Illegal byte sequence */
#define EUSERS   87  /* Too many users */
#define ENOTSOCK 88  /* Socket operation on non-socket */
#define EDESTADDRREQ 89 /* Destination address required */
#define EMSGSIZE 90  /* Message too long */
#define EPROTOTYPE 91 /* Protocol wrong type for socket */
#define ENOPROTOOPT 92 /* Protocol not available */
#define EPROTONOSUPPORT 93 /* Protocol not supported */
#define ESOCKTNOSUPPORT 94 /* Socket type not supported */
#define EOPNOTSUPP 95  /* Operation not supported */
#define EAFNOSUPPORT 97 /* Address family not supported */
#define EADDRINUSE 98  /* Address already in use */
#define EADDRNOTAVAIL 99 /* Cannot assign requested address */
#define ENETDOWN 100 /* Network is down */
#define ENETUNREACH 101 /* Network is unreachable */
#define ENETRESET 102 /* Network dropped connection */
#define ECONNABORTED 103 /* Software caused connection abort */
#define ECONNRESET 104 /* Connection reset by peer */
#define ENOBUFS  105 /* No buffer space available */
#define EISCONN  106 /* Transport endpoint is already connected */
#define ENOTCONN 107 /* Transport endpoint is not connected */
#define ETIMEDOUT 110 /* Connection timed out */
#define ECONNREFUSED 111 /* Connection refused */
#define EHOSTUNREACH 113 /* No route to host */
#define EALREADY 114 /* Operation already in progress */
#define EINPROGRESS 115 /* Operation now in progress */

#ifdef __cplusplus
}
#endif
