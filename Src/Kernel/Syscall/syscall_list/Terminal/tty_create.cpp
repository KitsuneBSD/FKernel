#include <Kernel/Driver/Terminal/terminal_manager.h>
#include <Kernel/Memory/UserAccess/user_access.h>
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

    char name_hint_buf[64];
    const char* name_hint = nullptr;
    if (name_hint_ptr != 0 && fkernel::memory::is_user_address(name_hint_ptr, 1)) {
        size_t len = 0;
        for (; len < sizeof(name_hint_buf) - 1; ++len) {
            char c;
            if (fkernel::memory::copy_from_user(&c, reinterpret_cast<const void*>(name_hint_ptr + len), 1).is_error())
                break;
            name_hint_buf[len] = c;
            if (c == '\0') break;
        }
        name_hint_buf[sizeof(name_hint_buf) - 1] = '\0';
        name_hint = name_hint_buf;
    }

    auto result = TerminalManager::the().create_terminal(static_cast<TerminalType>(type), name_hint);
    if (result.is_error()) {
        return -static_cast<uint64_t>(result.error());
    }

    return static_cast<uint64_t>(result.value().value());
}
