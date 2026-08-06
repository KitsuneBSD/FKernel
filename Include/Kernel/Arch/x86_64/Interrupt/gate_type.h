#pragma once

#include <LibFK/Types/types.h>

enum class GateType : uint8_t {
  InterruptGate = 0x8E, // P=1, DPL=0, Type=Interrupt (14)
  TrapGate      = 0x8F, // P=1, DPL=0, Type=Trap (15)
  UserTrapGate  = 0xEF, // P=1, DPL=3, Type=Trap (15) — SDM §6.11: int3/int1 from ring 3
};
