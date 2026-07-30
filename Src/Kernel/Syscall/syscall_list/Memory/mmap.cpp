#include <Kernel/Syscall/syscall_utils.h>
#include <Kernel/Arch/x86_64/Syscall/syscall_arch.h>
#include <Kernel/Arch/x86_64/arch_defs.h>
#include <Kernel/Syscall/syscall.h>
#include <Kernel/Syscall/syscall_utils.h>
#include <Kernel/Scheduler/scheduler.h>
#include <Kernel/Scheduler/Task/task.h>
#include <Kernel/Memory/VirtualMemory/virtual_memory_manager.h>
#include <Kernel/Memory/VirtualMemory/Pages/page_flags.h>
#include <Kernel/Memory/VirtualMemory/memory_region.h>
#include <Kernel/Memory/PhysicalMemory/physical_memory_manager.h>
#include <Kernel/Fs/Virtual/ShmFs/shm_node.h>
#include <Kernel/Fs/Vfs/file_description.h>
#include <Kernel/Fs/Vfs/node.h>
#include <LibFK/Algorithms/log.h>
#include <LibFK/Utilities/memory.h>

static constexpr uint64_t PROT_WRITE = 0x2;
static constexpr uint64_t PROT_EXEC  = 0x4;
static constexpr uint64_t MAP_ANONYMOUS = 0x20;
static constexpr uint64_t MAP_SHARED    = 0x01;
static constexpr uint64_t MAP_FIXED     = 0x10;

static fk::RefPtr<fkernel::ShmNode> resolve_shm(Task* task, int fd) {
  if (fd < 0) return nullptr;
  auto desc = task->get_file_descriptor(fd);
  if (!desc) return nullptr;
  auto node = desc->node();
  if (!node || !node->is_shm()) return nullptr;
  return fk::RefPtr<fkernel::ShmNode>(static_cast<fkernel::ShmNode*>(node.get()));
}

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

static PageFlags prot_to_page_flags(uint64_t prot) {
    PageFlags flags = PageFlags::Present | PageFlags::User;
    if (prot & PROT_WRITE) flags = flags | PageFlags::Writable;
    if (!(prot & PROT_EXEC)) flags = flags | PageFlags::ExecuteDisable;
    return flags;
}

static uint64_t mmap_file(Task* task, uintptr_t addr, uint64_t len, uint64_t prot,
                           uint64_t fd, uint64_t offset, bool is_fixed) {
    auto file = task->get_file_descriptor(static_cast<int>(fd));
    if (!file) {
        fk::algorithms::kwarn("MMAP", "invalid fd=%lu", fd);
        return fkernel::return_error(fk::core::Error::InvalidParameter);
    }

    uintptr_t target = reserve_mmap_range(task, addr, len, is_fixed);
    PageFlags flags = prot_to_page_flags(prot);
    uint64_t pages = (len + 0xFFF) >> 12;

    for (uint64_t i = 0; i < pages; ++i) {
        uintptr_t phys = PhysicalMemoryManager::the().alloc_page();
        if (!phys) return fkernel::return_error(fk::core::Error::OutOfMemory);
        fk::memory::set(reinterpret_cast<void*>(phys + KERNEL_VIRT_BASE), 0, 4096);
        VirtualMemoryManager::the().map_page(target + i * 4096, phys, flags);
    }

    uint8_t* dest = reinterpret_cast<uint8_t*>(target);
    size_t remaining = static_cast<size_t>(len);
    uint64_t file_offset = offset;
    size_t done = 0;

    while (done < remaining) {
        size_t chunk = remaining - done;
        if (chunk > 4096) chunk = 4096;
        auto res = file->node()->read(file_offset + done, chunk, dest + done);
        if (res.is_error()) break;
        size_t read = res.value();
        if (read == 0) break;
        done += read;
    }

    return target;
}

extern "C" {

uint64_t sys_mmap(uint64_t addr, uint64_t len, uint64_t prot, uint64_t flags,
                  uint64_t fd, uint64_t offset, [[maybe_unused]] PtRegs* regs) {
    auto* task = SchedulerManager::the().current();
    if (!task) return fkernel::return_error(fk::core::Error::PermissionDenied);

    if (flags & MAP_ANONYMOUS) {
        bool is_fixed = (flags & MAP_FIXED) != 0;
        uintptr_t target_addr = reserve_mmap_range(task, addr, len, is_fixed);
        PageFlags pg_flags = prot_to_page_flags(prot);
        uint64_t page_aligned_len = (len + 0xFFF) & ~0xFFFULL;

        fkernel::MemoryRegion region;
        region.start = target_addr;
        region.end = target_addr + page_aligned_len;
        region.flags = pg_flags;
        region.name = "anon";

        task->resources.memory.regions.list.push_back(region);

        return target_addr;
    }

    if ((flags & MAP_SHARED) && fd != (uint64_t)-1) {
        auto shm = resolve_shm(task, static_cast<int>(fd));
        if (shm) {
            bool is_fixed = (flags & MAP_FIXED) != 0;
            uintptr_t target_addr = reserve_mmap_range(task, addr, len, is_fixed);
            PageFlags pg_flags = prot_to_page_flags(prot);
            shm->map_into(task, target_addr, pg_flags);
            return target_addr;
        }
        // File-backed MAP_SHARED: read into private pages, track for writeback
        bool is_fixed = (flags & MAP_FIXED) != 0;
        uint64_t target = mmap_file(task, addr, len, prot, fd, offset, is_fixed);
        if (target < (uint64_t)-4096ULL) {
            auto file = task->get_file_descriptor(static_cast<int>(fd));
            if (file && file->node()) {
                FileMmapRecord rec;
                rec.vaddr       = static_cast<uintptr_t>(target);
                rec.size        = static_cast<size_t>(len);
                rec.node        = file->node();
                rec.file_offset = offset;
                task->resources.memory.file_mmaps.push_back(fk::types::move(rec));
            }
        }
        return target;
    }

    bool is_fixed = (flags & MAP_FIXED) != 0;
    return mmap_file(task, addr, len, prot, fd, offset, is_fixed);
}

uint64_t sys_munmap(uint64_t addr, uint64_t length, [[maybe_unused]] uint64_t, uint64_t, uint64_t, uint64_t, PtRegs*) {
    auto result = VirtualMemoryManager::the().munmap(addr, length);
    if (result.is_error()) {
        return fkernel::return_error(result.error());
    }
    return 0;
}

uint64_t sys_mremap(uint64_t old_addr, uint64_t old_size, uint64_t new_size,
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

}
