#pragma once

#include <LibFK/Types/types.h>

namespace fkernel {

enum class IoCompletionStatus : uint8_t { Success, Error, Timeout, Busy };

} // namespace fkernel
