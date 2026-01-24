#pragma once

#include <LibFK/Core/Error.h>
#include <Kernel/Posix/sys/errno.h>

namespace fkernel {

inline int error_to_errno(fk::core::Error error) {
    switch (error) {
        case fk::core::Error::None:
            return 0;
        case fk::core::Error::OutOfMemory:
            return ENOMEM;
        case fk::core::Error::InvalidParameter:
            return EINVAL;
        case fk::core::Error::NotFound:
            return ENOENT;
        case fk::core::Error::NotImplemented:
            return ENOSYS;
        case fk::core::Error::PermissionDenied:
            return EACCES;
        case fk::core::Error::IsDirectory:
            return EISDIR;
        case fk::core::Error::NotADirectory:
            return ENOTDIR;
        case fk::core::Error::IOError:
            return EIO;
        case fk::core::Error::Interrupted:
            return EINTR;
        case fk::core::Error::NoChildProcesses:
            return ECHILD;
        case fk::core::Error::InappropriateIoctlForDevice:
            return ENOTTY;
        case fk::core::Error::EndOfFile:
            return 0; // Usually not an error in POSIX read, but 0 return
        default:
            return EINVAL;
    }
}

inline uint64_t return_error(fk::core::Error error) {
    return (uint64_t)(-error_to_errno(error));
}

} // namespace fkernel
