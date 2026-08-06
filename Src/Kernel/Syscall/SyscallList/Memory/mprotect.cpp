#include <Kernel/Arch/x86_64/Syscall/syscall_arch.h>
#include <Kernel/Memory/VirtualMemory/virtual_memory_manager.h>
#include <Kernel/Memory/VirtualMemory/Pages/page_flags.h>
#include <Kernel/Scheduler/Core/scheduler.h>
#include <Kernel/Syscall/syscall_utils.h>

extern "C" uint64_t sys_mprotect(uint64_t addr, uint64_t len, uint64_t prot,
                                  uint64_t, uint64_t, uint64_t,
                                  [[maybe_unused]] PtRegs* regs) {
    if (addr & 0xFFF) return (uint64_t)-22;
    if (len == 0) return 0;

    uint64_t end = (addr + len + 0xFFF) & ~0xFFFULL;

    PageFlags flags = Present | User;
    if (prot & 2) flags = flags | Writable;
    if (!(prot & 4)) flags = flags | ExecuteDisable;

    auto& vmm = VirtualMemoryManager::the();
    for (uint64_t page = addr; page < end; page += 4096)
        vmm.protect_page(page, flags);
    return 0;
}
