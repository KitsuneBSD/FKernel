#include <Kernel/Arch/x86_64/Cpu/State.h>
#include <Kernel/Arch/x86_64/Interrupts/Exceptions.h>
#include <Kernel/Arch/x86_64/Interrupts/Interrupt_Constants.h>
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
    FK::enforcef(frame != nullptr, "Global default Handler: CpuState frame is nullptr");

    if (frame->interrupt_id >= MAX_EXCEPTION_ID) {
        FK::alert_if_f(true, "Global Default Handler: Unknown exception interrupt_id %u", frame->interrupt_id);
        return;
    }

    if (frame->interrupt_id == 13) {
        general_protection_fault_handler(frame);
    }

    FK::alert_if_f(frame->rip == 0, "Global default handler: RIP is NULL");
    FK::alert_if_f(frame->rsp == 0, "Global default handler: RSP is NULL");
    FK::alert_if_f(frame->rbp == 0, "Global default handler: RBP is NULL");

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
