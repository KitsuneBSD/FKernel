#pragma once

#include <LibFK/Types/types.h>

namespace fkernel {

enum class PCIEventType : uint8_t {
  Insertion = 1,
  Removal = 0
};

} // namespace fkernel
