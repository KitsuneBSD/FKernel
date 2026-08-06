#include <LibFK/Algorithms/Logging/log.h>

#include <Kernel/Driver/Terminal/terminal_manager.h>
#include <Kernel/Scheduler/Core/scheduler.h>
#include <Kernel/Syscall/syscall.h>
#include <Kernel/Syscall/syscall_numbers.h>

using namespace fkernel;
using namespace fkernel::terminal;

extern "C" uint64_t sys_tty_list_kernel(uint64_t terminal_ids_ptr, uint64_t max_count, uint64_t, uint64_t, uint64_t, uint64_t, PtRegs*) {
    auto* task = SchedulerManager::the().current();
    if (!task) {
        return -static_cast<uint64_t>(fk::core::Error::PermissionDenied);
    }
    
    if (terminal_ids_ptr == 0 || max_count == 0) {
        return -static_cast<uint64_t>(fk::core::Error::InvalidParameter);
    }
    
    auto* terminal_ids = reinterpret_cast<uint32_t*>(terminal_ids_ptr);
    
    size_t count = 0;
    
    // List VGA terminals (generate sequential IDs starting from 1)
    for (size_t i = 0; i < TerminalManager::the().vga_terminal_count() && count < max_count; ++i) {
        terminal_ids[count++] = static_cast<uint32_t>(i + 1);
    }
    
    return static_cast<uint64_t>(count);
}
