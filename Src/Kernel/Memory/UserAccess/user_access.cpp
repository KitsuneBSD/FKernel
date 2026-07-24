#include <Kernel/Memory/UserAccess/user_access.h>
#include <Kernel/Arch/x86_64/Hardware/Cpu/cpu_ops.h>
#include <Kernel/Hardware/Cpu/cpu.h>
#include <LibFK/Utilities/memory.h>

namespace fkernel {
namespace memory {

static void stac_if_smap() {
    if (CPU::the().has_smap())
        arch_smap_begin();
}

static void clac_if_smap() {
    if (CPU::the().has_smap())
        arch_smap_end();
}

fk::core::Result<void, fk::core::Error> copy_from_user(void* dst, const void* user_src, size_t n) {
    if (!is_user_address(reinterpret_cast<uintptr_t>(user_src), n))
        return fk::core::Error::InvalidParameter;
    stac_if_smap();
    fk::memory::copy(dst, user_src, n);
    clac_if_smap();
    return {};
}

fk::core::Result<void, fk::core::Error> copy_to_user(void* user_dst, const void* src, size_t n) {
    if (!is_user_address(reinterpret_cast<uintptr_t>(user_dst), n))
        return fk::core::Error::InvalidParameter;
    stac_if_smap();
    fk::memory::copy(user_dst, src, n);
    clac_if_smap();
    return {};
}

} // namespace memory
} // namespace fkernel
