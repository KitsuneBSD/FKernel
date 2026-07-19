#include <LibC/string.h>
#include <LibC/errno.h>

char *strerror(int errnum) {
  switch (errnum) {
    case EPERM:    return "Operation not permitted";
    case ENOENT:   return "No such file or directory";
    case ESRCH:    return "No such process";
    case EINTR:    return "Interrupted system call";
    case EIO:      return "I/O error";
    case ENXIO:    return "No such device or address";
    case E2BIG:    return "Argument list too long";
    case ENOEXEC:  return "Exec format error";
    case EBADF:    return "Bad file descriptor";
    case ECHILD:   return "No child processes";
    case EAGAIN:   return "Resource temporarily unavailable";
    case ENOMEM:   return "Cannot allocate memory";
    case EACCES:   return "Permission denied";
    case EFAULT:   return "Bad address";
    case EBUSY:    return "Device or resource busy";
    case EEXIST:   return "File exists";
    case ENODEV:   return "No such device";
    case ENOTDIR:  return "Not a directory";
    case EISDIR:   return "Is a directory";
    case EINVAL:   return "Invalid argument";
    case ENFILE:   return "File table overflow";
    case EMFILE:   return "Too many open files";
    case ENOTTY:   return "Not a typewriter";
    case EFBIG:    return "File too large";
    case ENOSPC:   return "No space left on device";
    case ESPIPE:   return "Illegal seek";
    case EROFS:    return "Read-only file system";
    case EPIPE:    return "Broken pipe";
    case ENOSYS:   return "Function not implemented";
    case ENOTEMPTY: return "Directory not empty";
    case ELOOP:    return "Too many symbolic links";
    case ENAMETOOLONG: return "File name too long";
    case ETIMEDOUT: return "Connection timed out";
    case ECONNREFUSED: return "Connection refused";
    case ECONNRESET: return "Connection reset by peer";
    default:       return "Unknown error";
  }
}
