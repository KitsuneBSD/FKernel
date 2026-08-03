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

// sys_munmap(...) → 0 or -errno
extern "C" uint64_t sys_munmap(uint64_t addr, uint64_t length, [[maybe_unused]] uint64_t, uint64_t, uint64_t, uint64_t, PtRegs*) {
    auto result = VirtualMemoryManager::the().munmap(addr, length);
    if (result.is_error()) {
        return fkernel::return_error(result.error());
    }
    return 0;
}
