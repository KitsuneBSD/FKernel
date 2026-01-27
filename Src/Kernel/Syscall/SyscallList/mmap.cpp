#include <Kernel/Arch/x86_64/Syscall/syscall_arch.h>
#include <Kernel/Syscall/syscall.h>
#include <Kernel/Syscall/syscall_utils.h>
#include <Kernel/Scheduler/scheduler.h>
#include <Kernel/Memory/VirtualMemory/virtual_memory_manager.h>
#include <Kernel/Memory/PhysicalMemory/physical_memory_manager.h>
#include <Kernel/Memory/VirtualMemory/Pages/page_flags.h>
#include <LibFK/Algorithms/log.h>
#include <LibC/string.h>

extern "C" {

uint64_t sys_mmap(uint64_t addr, uint64_t len, [[maybe_unused]] uint64_t prot, uint64_t flags,
                  uint64_t fd, [[maybe_unused]] uint64_t offset, PtRegs* regs) {
    auto* task = SchedulerManager::the().current();
    if (!task) return fkernel::return_error(fk::core::Error::PermissionDenied);

    bool anonymous = (flags & 0x20);
    if (!anonymous) {
        fk::algorithms::kwarn("SYSCALL", "sys_mmap: file-backed mapping not supported (fd=%ld)", fd);
        return fkernel::return_error(fk::core::Error::NotImplemented);
    }

    uintptr_t target_addr = addr;
    if (target_addr == 0) {
        target_addr = task->memory_regions.mmap_end;
        task->memory_regions.mmap_end += (len + 0xFFF) & ~0xFFFULL;
    }

    // No need to map anything here. The PF handler will take care of it.
    // We just return the reserved address.
    return target_addr;
}

uint64_t sys_munmap(uint64_t addr, uint64_t len, uint64_t, uint64_t, uint64_t, uint64_t, PtRegs* regs) {
    // FIXME: Implement page unmapping
    fk::algorithms::klog("SYSCALL", "munmap(%p, %lu) stub called", (void*)addr, len);
    return 0;
}

}