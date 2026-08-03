#pragma once

#include <Kernel/Fs/Vfs/Mount/fstab_entry.h>
#include <LibFK/Container/Sequence/vector.h>
#include <LibFK/Text/string.h>

namespace fkernel {
namespace fs {

class Fstab {
public:
  static fk::core::Result<fk::containers::Vector<FstabEntry>, fk::core::Error>
  parse(const char *content);
};

} // namespace fs
} // namespace fkernel
