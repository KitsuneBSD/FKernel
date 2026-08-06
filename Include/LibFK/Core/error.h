#pragma once

#include <LibC/errno.h>
#include <LibC/stdint.h>

namespace fk {
namespace core {

enum class Error {
  None = 0,
  PermissionDenied = 1,             // EPERM
  NotFound = 2,                     // ENOENT
  IOError = 5,                      // EIO
  DeviceError = 6,                  // ENXIO
  InvalidParameter = 22,            // EINVAL
  OutOfMemory = 12,                 // ENOMEM
  InvalidHandle = 9,                // EBADF
  IsDirectory = 21,                 // EISDIR
  NotADirectory = 20,               // ENOTDIR
  EndOfFile = 999,                  // Custom (no real equivalent for EOF in errno)
  InvalidData = 1001,               // fk-specific, distinct from EINVAL (100 = Linux ENETDOWN)
  Interrupted = 4,                  // EINTR
  Fault = 14,                       // EFAULT
  NoChildProcesses = 10,            // ECHILD
  InappropriateIoctlForDevice = 25, // ENOTTY
  NoSpaceLeftOnDevice = 28,         // ENOSPC
  NotASymlink = 1000,               // fk-specific, distinct from EINVAL (101 = Linux ENETUNREACH)
  IsASymlink = 40,                  // ELOOP or EINVAL
  NotImplemented = 38,              // ENOSYS
  AlreadyExists = 17,               // EEXIST
  DeviceBusy = 16,                  // EBUSY
  Timeout = 110,                    // ETIMEDOUT
  DirectoryNotEmpty = 39,           // ENOTEMPTY
  BrokenPipe = 32,                  // EPIPE
  WouldBlock = 11,                  // EAGAIN
  NotConnected = 107,               // ENOTCONN
  ReadOnly = 30,                    // EROFS
  NotSupported = 95,                // EOPNOTSUPP
  ProtocolNotSupported = 92,        // ENOPROTOOPT
  NoDevice = 19,                    // ENODEV
};

} // namespace core
} // namespace fk
