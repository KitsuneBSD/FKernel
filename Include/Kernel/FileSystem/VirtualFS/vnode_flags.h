#pragma once

#include <LibFK/Types/types.h>

/**
 * @brief Flags para VNode
 */
enum VNodeFlags : uint32_t {
  NONE = 0,
  READONLY = 1 << 0,
  EXECUTABLE = 1 << 1,
};
