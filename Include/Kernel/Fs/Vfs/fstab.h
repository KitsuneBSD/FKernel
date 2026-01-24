#pragma once

#include <LibFK/Container/vector.h>
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

class Fstab {
public:
  static fk::core::Result<fk::containers::Vector<FstabEntry>, fk::core::Error>
  parse(const char *content);
};

} // namespace fs
} // namespace fkernel
