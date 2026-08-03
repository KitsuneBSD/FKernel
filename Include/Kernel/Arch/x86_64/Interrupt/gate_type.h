#pragma once

#include <LibFK/Types/types.h>

enum class GateType : uint8_t {
  InterruptGate = 0x8E,
  TrapGate = 0x8F
};
