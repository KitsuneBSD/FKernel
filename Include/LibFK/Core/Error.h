#pragma once

#include <LibC/stdint.h>
#include <Kernel/Posix/sys/errno.h>

namespace fk {
namespace core {

enum class Error {
  None = 0,
  PermissionDenied = 1,  // EPERM
  NotFound = 2,          // ENOENT
  IOError = 5,           // EIO
  DeviceError = 6,       // ENXIO
  InvalidParameter = 22, // EINVAL
  OutOfMemory = 12,      // ENOMEM
  InvalidHandle = 9,     // EBADF
  IsDirectory = 21,      // EISDIR
  NotADirectory = 20,    // ENOTDIR
  EndOfFile = 999,       // Custom (no real equivalent for EOF in errno)
  InvalidData = 22,      // Same as EINVAL
  Interrupted = 4,       // EINTR
  NoChildProcesses = 10, // ECHILD
  InappropriateIoctlForDevice = 25, // ENOTTY
  NoSpaceLeftOnDevice = 28,         // ENOSPC
  NotASymlink = 22,                 // Same as EINVAL
  IsASymlink = 40,                  // ELOOP or EINVAL
  NotImplemented = 38,              // ENOSYS
  AlreadyExists = 17,               // EEXIST
};

} // namespace core
} // namespace fk

