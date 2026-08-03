#pragma once

#include <LibFK/Types/types.h>

namespace fkernel::scheduler {

enum class QoSClass : uint8_t {
    UserInteractive  = 0,
    UserInitiated    = 1,
    Default          = 2,
    Utility          = 3,
    Background       = 4,
    Maintenance      = 5,
};

} // namespace fkernel::scheduler
