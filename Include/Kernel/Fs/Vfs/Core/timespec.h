#pragma once

#include <LibFK/Types/types.h>

namespace fkernel {

struct timespec {
    int64_t tv_sec;
    int64_t tv_nsec;
};

} // namespace fkernel
