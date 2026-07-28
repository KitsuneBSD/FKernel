#include <Kernel/Arch/x86_64/Interrupt/Handler/exception_macros.h>
#include <Kernel/Hardware/Cpu/cpu.h>
#include <Kernel/Scheduler/scheduler.h>
#include <LibFK/Algorithms/log.h>

void device_not_available_handler(uint8_t vector, InterruptFrame* frame) {
    if (!frame) halt_forever();

    if ((frame->cs & 3) == 3) {
        auto* task = SchedulerManager::the().current();
        if (task && !task->is_a_kernel_task()) {
            asm volatile("clts");
            return;
        }
    }

    fk::algorithms::kexception(
        "Device Not Available",
        "vector=%u RIP=%p RSP=%p RFLAGS=%p cs=0x%lx",
        (unsigned)vector, frame->rip, frame->rsp, frame->rflags, (unsigned long)frame->cs
    );
    halt_forever();
}
