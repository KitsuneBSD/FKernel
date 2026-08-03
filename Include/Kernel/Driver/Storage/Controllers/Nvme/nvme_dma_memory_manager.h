#pragma once

#include <LibFK/Core/result.h>
#include <LibFK/Types/types.h>

namespace fkernel {

class NvmeDmaMemoryManager {
public:
  static fk::core::Result<uintptr_t, fk::core::Error> allocate(size_t size);

  static void free(uintptr_t phys_addr, size_t size);
};

} // namespace fkernel
