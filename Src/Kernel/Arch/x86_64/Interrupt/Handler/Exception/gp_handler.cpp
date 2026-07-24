#include <Kernel/Arch/x86_64/Interrupt/Handler/exception_macros.h>
#include <Kernel/Arch/x86_64/Interrupt/Handler/handlers.h>
#include <Kernel/Ipc/signal_delivery.h>
#include <Kernel/Memory/UserAccess/user_access.h>
#include <Kernel/Posix/signal_defs.h>
#include <Kernel/Scheduler/scheduler.h>
#include <LibFK/Algorithms/log.h>

void general_protection_handler(uint8_t vector, InterruptFrame* frame) {
    if (!frame) halt_forever();

    // User-mode #GP: check for privileged instructions (hlt, invd, wbinvd, etc.)
    // that musl/busybox use in user space. Linux converts these to SIGILL.
    if ((frame->cs & 3) == 3) {
        uint8_t insn = 0;
        if (fkernel::memory::copy_from_user(&insn,
                reinterpret_cast<const void*>(frame->rip), 1).is_ok()) {
            // hlt (0xF4), invd (0x08), wbinvd (0x09) — privileged instructions
            // that user-space code (musl __lock) may execute as hints.
            if (insn == 0xF4 || insn == 0x08 || insn == 0x09) {
                auto* task = SchedulerManager::the().current();
                if (task && !task->is_a_kernel_task()) {
                    fk::algorithms::klog("GPF",
                        "User-mode privileged insn 0x%02x at RIP=%p, sending SIGILL",
                        (unsigned)insn, (void*)frame->rip);
                    fkernel::ipc::SignalDelivery::send_signal(task, SIGILL);
                    return;
                }
            }
        }
    }

    // Kernel-mode #GP or unrecognized user-mode #GP — fatal
    fk::algorithms::kexception(
        "General Protection",
        "vector=%u error=%p RIP=%p RSP=%p RFLAGS=%p",
        (unsigned)vector, frame->error_code,
        (void*)frame->rip, (void*)frame->rsp, (void*)frame->rflags
    );
    fk::algorithms::kexception(
        "General Protection",
        "  rax=%p rbx=%p rcx=%p rdx=%p",
        (void*)frame->rax, (void*)frame->rbx, (void*)frame->rcx, (void*)frame->rdx
    );
    fk::algorithms::kexception(
        "General Protection",
        "  rsi=%p rdi=%p rbp=%p r8=%p",
        (void*)frame->rsi, (void*)frame->rdi, (void*)frame->rbp, (void*)frame->r8
    );
    fk::algorithms::kexception(
        "General Protection",
        "  r9=%p r10=%p r11=%p r12=%p r13=%p r14=%p r15=%p",
        (void*)frame->r9, (void*)frame->r10, (void*)frame->r11,
        (void*)frame->r12, (void*)frame->r13, (void*)frame->r14, (void*)frame->r15
    );

    halt_forever();
}
