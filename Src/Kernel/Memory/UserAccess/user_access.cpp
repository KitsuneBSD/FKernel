#include <LibFK/Utilities/memory.h>

#include <Kernel/Memory/UserAccess/user_access.h>
#include <Kernel/Arch/x86_64/Hardware/Cpu/cpu_ops.h>
#include <Kernel/Hardware/Cpu/cpu.h>
#include <Kernel/Scheduler/Core/scheduler.h>

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

static bool user_range_is_accessible(const void* ptr, size_t n) {
    if (n == 0)
        return true;
    Task* task = SchedulerManager::the().current();
    if (!task)
        return false;
    uintptr_t addr = reinterpret_cast<uintptr_t>(ptr);
    uintptr_t page = addr & ~0xFFFULL;
    uintptr_t last = (addr + n - 1) & ~0xFFFULL;
    while (page < last) {
        if (!task->is_address_in_allowed_regions(page))
            return false;
        page += PAGE_SIZE;
    }
    return task->is_address_in_allowed_regions(last);
}

fk::core::Result<void, fk::core::Error> copy_from_user(void* dst, const void* user_src, size_t n) {
    if (!is_user_address(reinterpret_cast<uintptr_t>(user_src), n))
        return fk::core::Error::InvalidParameter;
    if (!user_range_is_accessible(user_src, n))
        return fk::core::Error::Fault;
    stac_if_smap();
    fk::memory::copy(dst, user_src, n);
    clac_if_smap();
    return {};
}

fk::core::Result<void, fk::core::Error> copy_to_user(void* user_dst, const void* src, size_t n) {
    if (!is_user_address(reinterpret_cast<uintptr_t>(user_dst), n))
        return fk::core::Error::InvalidParameter;
    if (!user_range_is_accessible(user_dst, n))
        return fk::core::Error::Fault;
    stac_if_smap();
    fk::memory::copy(user_dst, src, n);
    clac_if_smap();
    return {};
}

} // namespace memory
} // namespace fkernel
