#pragma once

#include <Kernel/Arch/x86_64/arch_defs.h>
#include <LibFK/Core/result.h>
#include <LibFK/Types/types.h>

namespace fkernel {
namespace memory {

static constexpr uintptr_t USERSPACE_MAX = 0x0000800000000000ULL;

inline bool is_user_address(uintptr_t addr, size_t len) {
    // Reject NULL and first page (unmapped guard page).
    if (addr < PAGE_SIZE) return false;
    if (len > USERSPACE_MAX) return false;
    return addr < USERSPACE_MAX && (addr + len) <= USERSPACE_MAX;
}

fk::core::Result<void, fk::core::Error> copy_from_user(void* dst, const void* user_src, size_t n);
fk::core::Result<void, fk::core::Error> copy_to_user(void* user_dst, const void* src, size_t n);

// Copies a null-terminated string from user space into dst (max_len bytes including '\0').
// Validates accessibility page-by-page rather than upfront for the full max_len window.
// Returns the number of characters copied (excluding '\0') on success.
fk::core::Result<size_t, fk::core::Error> copy_string_from_user(char* dst, const void* user_src, size_t max_len);

} // namespace memory
} // namespace fkernel
