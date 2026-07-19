#include <Kernel/Memory/UserAccess/user_access.h>
#include <Kernel/Hardware/Cpu/cpu.h>
#include <LibC/string.h>

namespace fkernel {
namespace memory {

static void stac_if_smap() {
    if (CPU::the().has_smap())
        asm volatile("stac" ::: "memory");
}

static void clac_if_smap() {
    if (CPU::the().has_smap())
        asm volatile("clac" ::: "memory");
}

fk::core::Result<void, fk::core::Error> copy_from_user(void* dst, const void* user_src, size_t n) {
    if (!is_user_address(reinterpret_cast<uintptr_t>(user_src), n))
        return fk::core::Error::InvalidParameter;
    stac_if_smap();
    memcpy(dst, user_src, n);
    clac_if_smap();
    return {};
}

fk::core::Result<void, fk::core::Error> copy_to_user(void* user_dst, const void* src, size_t n) {
    if (!is_user_address(reinterpret_cast<uintptr_t>(user_dst), n))
        return fk::core::Error::InvalidParameter;
    stac_if_smap();
    memcpy(user_dst, src, n);
    clac_if_smap();
    return {};
}

} // namespace memory
} // namespace fkernel
