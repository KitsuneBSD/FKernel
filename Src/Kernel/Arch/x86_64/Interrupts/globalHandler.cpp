#include <Kernel/Arch/x86_64/Cpu/State.h>
#include <Kernel/Arch/x86_64/Interrupts/Exceptions.h>
#include <Kernel/Arch/x86_64/Interrupts/Routines.h>
#include <LibFK/log.h>

void halt()
{
    while (true) {
        asm("hlt");
    }
}

extern "C" void global_default_handler(CpuState* const frame)
{
    Logf(LogLevel::ERROR, "[%u] Exception %u (%s) at RIP=%p, ErrorCode=0x%x",
        uptime(),
        frame->interrupt_id,
        named_exception(frame->interrupt_id),
        reinterpret_cast<void*>(frame->rip),
        frame->error_code);

    Logf(LogLevel::ERROR,
        "RIP=%p RSP=%p RBP=%p RFLAGS=0x%llx",
        reinterpret_cast<void*>(frame->rip),
        reinterpret_cast<void*>(frame->rsp),
        reinterpret_cast<void*>(frame->rbp),
        frame->rflags);

    halt();
}
