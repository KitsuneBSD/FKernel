#pragma once

#include <LibFK/Core/Result.h>
#include <LibFK/Types/types.h>

namespace fkernel {
namespace memory {

static constexpr uintptr_t USERSPACE_MAX = 0x0000800000000000ULL;

inline bool is_user_address(uintptr_t addr, size_t len) {
    return addr < USERSPACE_MAX && (addr + len) <= USERSPACE_MAX;
}

fk::core::Result<void, fk::core::Error> copy_from_user(void* dst, const void* user_src, size_t n);
fk::core::Result<void, fk::core::Error> copy_to_user(void* user_dst, const void* src, size_t n);

} // namespace memory
} // namespace fkernel
