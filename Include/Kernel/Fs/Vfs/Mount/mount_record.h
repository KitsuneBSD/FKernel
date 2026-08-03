#pragma once

#include <LibFK/Types/types.h>

namespace fkernel {

class Dentry;

struct MountRecord {
  char path[128];
  char fstype[16];
  uint32_t dev_id;
  Dentry* dentry_ptr;
};

} // namespace fkernel
