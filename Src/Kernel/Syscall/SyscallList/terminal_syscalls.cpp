#include <Kernel/Driver/Terminal/terminal_manager.h>
#include <Kernel/Scheduler/scheduler.h>
#include <Kernel/Syscall/syscall.h>
#include <Kernel/Syscall/syscall_numbers.h>
#include <LibFK/Algorithms/log.h>

using namespace fkernel;
using namespace fkernel::terminal;

// Syscall wrappers for kernel interface
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