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

fk::core::Result<size_t, fk::core::Error> copy_string_from_user(char* dst, const void* user_src, size_t max_len) {
    if (!user_src || !dst || max_len == 0)
        return fk::core::Error::InvalidParameter;

    uintptr_t addr = reinterpret_cast<uintptr_t>(user_src);
    if (!is_user_address(addr, 1))
        return fk::core::Error::InvalidParameter;

    Task* task = SchedulerManager::the().current();
    if (!task)
        return fk::core::Error::InvalidParameter;

    const char* src = static_cast<const char*>(user_src);
    size_t copied = 0;

    stac_if_smap();
    while (copied < max_len - 1) {
        // Check accessibility at each page boundary.
        uintptr_t page = (addr + copied) & ~static_cast<uintptr_t>(0xFFF);
        if (!task->is_address_in_allowed_regions(page)) {
            clac_if_smap();
            return fk::core::Error::Fault;
        }

        // Copy up to end of this page or max_len, whichever is sooner.
        size_t page_offset = (addr + copied) & 0xFFF;
        size_t bytes_in_page = PAGE_SIZE - page_offset;
        size_t limit = bytes_in_page < (max_len - 1 - copied) ? bytes_in_page : (max_len - 1 - copied);

        for (size_t i = 0; i < limit; ++i) {
            char c = src[copied + i];
            dst[copied + i] = c;
            if (c == '\0') {
                clac_if_smap();
                return copied + i;
            }
        }
        copied += limit;
    }
    clac_if_smap();
    dst[max_len - 1] = '\0';
    return max_len - 1;
}

} // namespace memory
} // namespace fkernel
