#pragma once

namespace fk {
namespace core {

enum class Error {
  None,
  OutOfMemory,
  InvalidParameter,
  NotFound,
  NotImplemented,
  DeviceError,
  // Add more specific error types as needed
};

} // namespace core
} // namespace fk
