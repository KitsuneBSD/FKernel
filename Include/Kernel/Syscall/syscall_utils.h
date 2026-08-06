#pragma once

#include <LibFK/Core/error.h>
#include <LibFK/Types/types.h>

namespace fkernel {

inline int error_to_errno(fk::core::Error error) {
  switch (error) {
  case fk::core::Error::None:
    return 0;
  case fk::core::Error::OutOfMemory:
    return 12; // ENOMEM
  case fk::core::Error::InvalidParameter:
    return 22; // EINVAL
  case fk::core::Error::NotFound:
    return 2;  // ENOENT
  case fk::core::Error::NotImplemented:
    return 38; // ENOSYS
  case fk::core::Error::PermissionDenied:
    return 1;  // EPERM
  case fk::core::Error::IsDirectory:
    return 21; // EISDIR
  case fk::core::Error::NotADirectory:
    return 20; // ENOTDIR
  case fk::core::Error::IOError:
    return 5;  // EIO
  case fk::core::Error::Interrupted:
    return 4;  // EINTR
  case fk::core::Error::Fault:
    return 14; // EFAULT
  case fk::core::Error::NoChildProcesses:
    return 10; // ECHILD
  case fk::core::Error::InappropriateIoctlForDevice:
    return 25; // ENOTTY
  case fk::core::Error::EndOfFile:
    return 0; // Usually not an error in POSIX read, but 0 return
  case fk::core::Error::DeviceError:
    return 6;  // ENXIO
  case fk::core::Error::InvalidHandle:
    return 9;  // EBADF
  case fk::core::Error::WouldBlock:
    return 11; // EAGAIN
  case fk::core::Error::DeviceBusy:
    return 16; // EBUSY
  case fk::core::Error::AlreadyExists:
    return 17; // EEXIST
  case fk::core::Error::NoDevice:
    return 19; // ENODEV
  case fk::core::Error::NoSpaceLeftOnDevice:
    return 28; // ENOSPC
  case fk::core::Error::ReadOnly:
    return 30; // EROFS
  case fk::core::Error::BrokenPipe:
    return 32; // EPIPE
  case fk::core::Error::DirectoryNotEmpty:
    return 39; // ENOTEMPTY
  case fk::core::Error::IsASymlink:
    return 40; // ELOOP
  case fk::core::Error::ProtocolNotSupported:
    return 92; // ENOPROTOOPT
  case fk::core::Error::NotSupported:
    return 95; // EOPNOTSUPP
  case fk::core::Error::NotASymlink:
    return 22; // EINVAL (readlink(2) on non-symlink)
  case fk::core::Error::InvalidData:
    return 22; // EINVAL
  case fk::core::Error::NotConnected:
    return 107; // ENOTCONN
  case fk::core::Error::Timeout:
    return 110; // ETIMEDOUT
  default:
    return 22; // EINVAL
  }
}

inline uint64_t return_error(fk::core::Error error) {
  return (uint64_t)(-error_to_errno(error));
}

} // namespace fkernel
