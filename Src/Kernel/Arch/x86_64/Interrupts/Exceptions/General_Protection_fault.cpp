#include <Kernel/Arch/x86_64/Cpu/State.h>
#include <Kernel/Arch/x86_64/Interrupts/Exceptions.h>
#include <Kernel/Arch/x86_64/Interrupts/Routines.h>
#include <LibFK/enforce.h>

void general_protection_fault_handler(CpuState* frame)
{
    Logf(LogLevel::ERROR,
        "[%u] General Protection Fault (#GP) at RIP=%p",
        uptime(),
        reinterpret_cast<void*>(frame->rip));

    Logf(LogLevel::ERROR,
        "Error Code: 0x%x | CS: 0x%llx | RFLAGS: 0x%llx",
        frame->error_code,
        frame->cs,
        frame->rflags);

    Logf(LogLevel::ERROR,
        "RSP=%p RBP=%p",
        reinterpret_cast<void*>(frame->rsp),
        reinterpret_cast<void*>(frame->rbp));

    halt();
}
