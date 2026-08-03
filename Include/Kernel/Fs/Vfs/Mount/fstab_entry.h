#pragma once

#include <LibFK/Text/string.h>

namespace fkernel {
namespace fs {

struct FstabEntry {
  fk::text::String device;
  fk::text::String mountpoint;
  fk::text::String type;
  fk::text::String options;
  int dump;
  int pass;
};

} // namespace fs
} // namespace fkernel
