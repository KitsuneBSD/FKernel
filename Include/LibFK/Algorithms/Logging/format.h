#pragma once

#include <LibC/stdarg.h>
#include <LibC/stdio.h>
#include <LibFK/Types/types.h>

namespace fk {
namespace algorithms {

inline int format(char* buf, size_t size, const char* fmt, ...) {
  va_list args;
  va_start(args, fmt);
  int result = vsnprintf(buf, size, fmt, args);
  va_end(args);
  return result;
}

inline int format_to(char* buf, size_t size, const char* fmt, va_list args) {
  return vsnprintf(buf, size, fmt, args);
}

} // namespace algorithms
} // namespace fk
