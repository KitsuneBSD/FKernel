#pragma once

#include <LibFK/Core/Error.h>
#include <LibC/stdint.h>

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
    return 1;  // EACCES
  case fk::core::Error::IsDirectory:
    return 21; // EISDIR
  case fk::core::Error::NotADirectory:
    return 20; // ENOTDIR
  case fk::core::Error::IOError:
    return 5;  // EIO
  case fk::core::Error::Interrupted:
    return 4;  // EINTR
  case fk::core::Error::NoChildProcesses:
    return 10; // ECHILD
  case fk::core::Error::InappropriateIoctlForDevice:
    return 25; // ENOTTY
  case fk::core::Error::EndOfFile:
    return 0; // Usually not an error in POSIX read, but 0 return
  default:
    return 22; // EINVAL
  }
}

inline uint64_t return_error(fk::core::Error error) {
  return (uint64_t)(-error_to_errno(error));
}

} // namespace fkernel
