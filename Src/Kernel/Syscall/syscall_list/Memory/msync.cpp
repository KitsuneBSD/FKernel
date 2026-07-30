#include <Kernel/Arch/x86_64/Syscall/syscall_arch.h>
#include <Kernel/Arch/x86_64/arch_defs.h>
#include <Kernel/Memory/VirtualMemory/virtual_memory_manager.h>
#include <Kernel/Scheduler/scheduler.h>
#include <Kernel/Scheduler/Task/task.h>
#include <Kernel/Syscall/syscall.h>
#include <Kernel/Syscall/syscall_utils.h>

// x86_64 PTE dirty bit
static constexpr uint64_t PTE_DIRTY = (1ULL << 6);

static void writeback_region(FileMmapRecord& rec, uintptr_t start, uintptr_t end) {
    auto& vmm = VirtualMemoryManager::the();
    uintptr_t page_start = start & ~0xFFFULL;
    uintptr_t page_end   = (end + 0xFFF) & ~0xFFFULL;

    for (uintptr_t vaddr = page_start; vaddr < page_end; vaddr += 4096) {
        if (vaddr < rec.vaddr || vaddr >= rec.vaddr + rec.size) continue;
        uint64_t* pte = vmm.get_pte(vaddr);
        if (!pte || !(*pte & PTE_DIRTY)) continue;

        uint64_t file_off = rec.file_offset + (vaddr - rec.vaddr);
        const uint8_t* page_data = reinterpret_cast<const uint8_t*>(
            (*pte & ~0xFFFULL) + KERNEL_VIRT_BASE);

        (void)rec.node->write(file_off, 4096, page_data);
        *pte &= ~PTE_DIRTY; // clear dirty bit
        asm volatile("invlpg (%0)" :: "r"(vaddr) : "memory");
    }
}

extern "C" uint64_t sys_msync(uint64_t addr, uint64_t length, uint64_t flags,
                               uint64_t, uint64_t, uint64_t,
                               [[maybe_unused]] PtRegs* regs) {
    (void)flags;
    auto* task = SchedulerManager::the().current();
    if (!task) return fkernel::return_error(fk::core::Error::PermissionDenied);
    if (addr & 0xFFF) return fkernel::return_error(fk::core::Error::InvalidParameter);

    uintptr_t end = addr + length;
    auto& file_mmaps = task->resources.memory.file_mmaps;
    for (size_t i = 0; i < file_mmaps.size(); ++i) {
        auto& rec = file_mmaps[i];
        uintptr_t rec_end = rec.vaddr + rec.size;
        if (rec_end <= addr || rec.vaddr >= end) continue;
        uintptr_t overlap_start = rec.vaddr > addr ? rec.vaddr : addr;
        uintptr_t overlap_end   = rec_end < end ? rec_end : end;
        writeback_region(rec, overlap_start, overlap_end);
    }
    return 0;
}
