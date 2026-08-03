#pragma once

#include <LibFK/Types/types.h>

namespace fkernel::scheduler {

enum class SchedulingPolicy : uint8_t {
    Normal     = 0,
    Fifo       = 1,
    RoundRobin = 2,
    Batch      = 3,
    Idle       = 4,
};

} // namespace fkernel::scheduler
