#include <LibFK/Algorithms/Logging/log.h>

#include <Kernel/Driver/Terminal/terminal_manager.h>
#include <Kernel/Scheduler/Core/scheduler.h>
#include <Kernel/Syscall/syscall.h>
#include <Kernel/Syscall/syscall_numbers.h>

using namespace fkernel;
using namespace fkernel::terminal;

extern "C" uint64_t sys_tty_delete_kernel(uint64_t terminal_id, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, PtRegs*) {
    if (terminal_id == 0) {
        return -static_cast<uint64_t>(fk::core::Error::InvalidParameter);
    }
    
    TerminalId id(static_cast<uint32_t>(terminal_id));
    auto result = TerminalManager::the().delete_terminal(id);
    
    if (result.is_error()) {
        return -static_cast<uint64_t>(result.error());
    }
    
    return 0;
}
