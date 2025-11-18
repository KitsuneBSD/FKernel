#pragma once

#include <LibFK/Types/types.h>

namespace fk {
namespace algorithms {

uint32_t djb2(const void* data, size_t length);

} // namespace algorithms
} // namespace fk
