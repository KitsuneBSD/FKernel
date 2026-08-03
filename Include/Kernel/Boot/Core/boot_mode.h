#pragma once

#include <LibFK/Types/types.h>

namespace boot {

enum class BootMode : uint32_t {
  Unknown = 0,
  Multiboot2 = 1,
};

} // namespace boot
