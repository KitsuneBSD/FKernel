#pragma once

#include <LibFK/Types/types.h>

namespace fkernel {

struct kevent {
    uint64_t ident;
    int16_t filter;
    uint16_t flags;
    uint32_t fflags;
    int64_t data;
    void* udata;
};

} // namespace fkernel
