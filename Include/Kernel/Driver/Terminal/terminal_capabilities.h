#pragma once

#include <LibFK/Types/types.h>

namespace fkernel {
namespace terminal {

struct TerminalCapabilities {
    bool supports_color{false};
    bool supports_raw_mode{false};
    bool supports_canonical_mode{false};
    uint16_t max_rows{25};
    uint16_t max_cols{80};
};

} // namespace terminal
} // namespace fkernel
