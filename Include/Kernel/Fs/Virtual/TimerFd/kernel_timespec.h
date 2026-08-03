#pragma once

#include <LibFK/Types/types.h>

namespace fkernel {

struct KernelTimespec {
    int64_t tv_sec;
    int64_t tv_nsec;
};

} // namespace fkernel
