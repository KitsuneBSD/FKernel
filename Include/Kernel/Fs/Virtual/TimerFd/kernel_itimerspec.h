#pragma once

#include "kernel_timespec.h"

namespace fkernel {

struct KernelItimerspec {
    KernelTimespec it_interval;
    KernelTimespec it_value;
};

} // namespace fkernel
