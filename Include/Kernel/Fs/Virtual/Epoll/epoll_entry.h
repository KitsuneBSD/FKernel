#pragma once

#include <LibFK/Types/types.h>

namespace fkernel {

struct EpollEntry {
    int      fd;
    uint32_t events;
    uint64_t data_u64;
};

} // namespace fkernel
