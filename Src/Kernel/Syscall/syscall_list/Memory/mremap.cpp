#include <Kernel/Syscall/syscall_utils.h>
#include <Kernel/Arch/x86_64/Syscall/syscall_arch.h>
#include <Kernel/Arch/x86_64/arch_defs.h>
#include <Kernel/Syscall/syscall.h>
#include <Kernel/Scheduler/Core/scheduler.h>
#include <Kernel/Memory/VirtualMemory/virtual_memory_manager.h>
#include <Kernel/Memory/VirtualMemory/Pages/page_flags.h>
#include <Kernel/Memory/VirtualMemory/memory_region.h>
#include <Kernel/Memory/PhysicalMemory/physical_memory_manager.h>
#include <Kernel/Fs/Virtual/ShmFs/shm_node.h>
#include <Kernel/Fs/Vfs/Core/file_description.h>
#include <Kernel/Fs/Vfs/Core/node.h>
#include <LibFK/Algorithms/Logging/log.h>
#include <LibFK/Utilities/memory.h>

static uintptr_t reserve_mmap_range(Task* task, uintptr_t hint, uint64_t len, bool fixed) {
    if (fixed) {
        if (hint == 0 || (hint & 0xFFF) != 0)
            return 0; // EINVAL
        uintptr_t aligned_len = (len + 0xFFF) & ~0xFFFULL;
        uintptr_t end = hint + aligned_len;
        // If MAP_FIXED overlaps existing regions, unmap them
        auto& regions = task->resources.memory.regions.list;
        for (size_t i = 0; i < regions.size(); ) {
            auto& r = regions[i];
            if (r.start < end && hint < r.end) {
                VirtualMemoryManager::the().munmap(r.start, r.end - r.start);
                regions.remove_at(i);
            } else {
                ++i;
            }
        }
        return hint;
    }
    if (hint != 0) return hint;
    uintptr_t addr = task->memory().regions.mmap_end;
    task->memory().regions.mmap_end += (len + 0xFFF) & ~0xFFFULL;
    return addr;
}

// sys_mremap(...) → 0 or -errno
extern "C" uint64_t sys_mremap(uint64_t old_addr, uint64_t old_size, uint64_t new_size,
                    uint64_t flags, uint64_t new_addr_hint, uint64_t,
                    [[maybe_unused]] PtRegs* regs) {
    if (new_size == 0) return (uint64_t)-12;  // ENOMEM
    if (new_size <= old_size) return old_addr; // shrink in place

    auto* task = SchedulerManager::the().current();
    if (!task) return (uint64_t)-12;

    (void)flags; (void)new_addr_hint;
    uintptr_t new_region = reserve_mmap_range(task, 0, new_size, false);
    uint64_t new_pages = (new_size + 0xFFF) >> 12;
    for (uint64_t i = 0; i < new_pages; ++i) {
        uintptr_t phys = PhysicalMemoryManager::the().alloc_page();
        if (!phys) return (uint64_t)-12;
        VirtualMemoryManager::the().map_page(new_region + i * 4096, phys,
            PageFlags::Present | PageFlags::Writable | PageFlags::User);
        fk::memory::set(reinterpret_cast<void*>(new_region + i * 4096), 0, 4096);
    }
    fk::memory::copy(reinterpret_cast<void*>(new_region),
           reinterpret_cast<const void*>(old_addr),
           static_cast<size_t>(old_size));
    VirtualMemoryManager::the().munmap(old_addr, old_size);
    return new_region;
}
