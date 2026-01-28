#include <Kernel/Driver/Terminal/terminal_manager.h>
#include <Kernel/Scheduler/scheduler.h>
#include <Kernel/Syscall/syscall.h>
#include <Kernel/Syscall/syscall_numbers.h>
#include <LibFK/Algorithms/log.h>

using namespace fkernel;
using namespace fkernel::terminal;

extern "C" uint64_t sys_tty_create_kernel(uint64_t type, uint64_t name_hint_ptr, uint64_t, uint64_t, uint64_t, uint64_t, PtRegs*) {
    auto* task = SchedulerManager::the().current();
    if (!task) {
        return -static_cast<uint64_t>(fk::core::Error::PermissionDenied);
    }
    
    const char* name_hint = nullptr;
    if (name_hint_ptr != 0) {
        // Validate user pointer (simplified - should use proper validation)
        name_hint = reinterpret_cast<const char*>(name_hint_ptr);
    }
    
    auto result = TerminalManager::the().create_terminal(static_cast<TerminalType>(type), name_hint);
    if (result.is_error()) {
        return -static_cast<uint64_t>(result.error());
    }
    
    return static_cast<uint64_t>(result.value().value());
}
