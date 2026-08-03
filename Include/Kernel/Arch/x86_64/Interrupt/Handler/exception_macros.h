#pragma once

#include <Kernel/Arch/x86_64/Interrupt/Handler/handlers.h>
#include <Kernel/Ipc/Signals/signal_delivery.h>
#include <Kernel/Posix/signal_defs.h>
#include <Kernel/Scheduler/Core/scheduler.h>
#include <LibFK/Algorithms/Logging/log.h>
#include <LibFK/Core/assertions.h>
#include <LibFK/Types/types.h>

[[noreturn]] inline void halt_forever() {
    for (;;) {
        asm volatile("cli; hlt");
    }
}

#define GENERIC_EXCEPTION_HANDLER(name, exception_message)            \
void name(uint8_t vector, InterruptFrame* frame) {                   \
    if (!frame) halt_forever();                                        \
                                                                        \
    fk::algorithms::kexception(                                      \
        exception_message,                                           \
        "vector=%u RIP=%p RSP=%p RFLAGS=%p",                          \
        (unsigned)vector, frame->rip, frame->rsp, frame->rflags       \
    );                                                                 \
                                                                        \
    halt_forever();                                                    \
}

#define GENERIC_EXCEPTION_HANDLER_WITH_ERROR_CODE(name, exception_message) \
void name(uint8_t vector, InterruptFrame* frame) {                         \
    if (!frame) halt_forever();                                              \
                                                                              \
    fk::algorithms::kexception(                                            \
        exception_message,                                                 \
        "vector=%u error=%p RIP=%p RSP=%p RFLAGS=%p",                      \
        (unsigned)vector, frame->error_code,                                \
        frame->rip, frame->rsp, frame->rflags                               \
    );                                                                       \
    fk::algorithms::kexception(                                            \
        exception_message,                                                 \
        "  rax=%p rbx=%p rcx=%p rdx=%p",                                   \
        frame->rax, frame->rbx, frame->rcx, frame->rdx                     \
    );                                                                       \
    fk::algorithms::kexception(                                            \
        exception_message,                                                 \
        "  rsi=%p rdi=%p rbp=%p r8=%p",                                    \
        frame->rsi, frame->rdi, frame->rbp, frame->r8                      \
    );                                                                       \
    fk::algorithms::kexception(                                            \
        exception_message,                                                 \
        "  r9=%p r10=%p r11=%p r12=%p r13=%p r14=%p r15=%p",              \
        frame->r9, frame->r10, frame->r11,                                  \
        frame->r12, frame->r13, frame->r14, frame->r15                     \
    );                                                                       \
                                                                              \
    halt_forever();                                                          \
}

#define GENERIC_EXCEPTION_HANDLER_USER(name, exception_message, signal, si_code_val) \
void name(uint8_t vector, InterruptFrame* frame) {                                    \
    if (!frame) halt_forever();                                                         \
                                                                                         \
    if ((frame->cs & 3) == 3) {                                                         \
        auto* task = SchedulerManager::the().current();                                  \
        if (task && !task->is_a_kernel_task()) {                                         \
            siginfo_t si{};                                                              \
            si.si_signo = (signal);                                                      \
            si.si_code  = (si_code_val);                                                 \
            si.si_addr  = frame->rip;                                                    \
            fkernel::ipc::SignalDelivery::send_signal(task, (signal), &si, true);         \
            return;                                                                      \
        }                                                                                \
    }                                                                                    \
                                                                                          \
    fk::algorithms::kexception(                                                       \
        exception_message,                                                            \
        "vector=%u RIP=%p RSP=%p RFLAGS=%p cs=0x%lx",                                  \
        (unsigned)vector, frame->rip, frame->rsp, frame->rflags, (unsigned long)frame->cs \
    );                                                                                  \
                                                                                          \
    halt_forever();                                                                     \
}

#define GENERIC_EXCEPTION_HANDLER_USER_WITH_ERROR(name, exception_message, signal, si_code_val) \
void name(uint8_t vector, InterruptFrame* frame) {                                      \
    if (!frame) halt_forever();                                                           \
                                                                                           \
    if ((frame->cs & 3) == 3) {                                                           \
        auto* task = SchedulerManager::the().current();                                    \
        if (task && !task->is_a_kernel_task()) {                                           \
            siginfo_t si{};                                                                \
            si.si_signo = (signal);                                                        \
            si.si_code  = (si_code_val);                                                   \
            si.si_addr  = frame->rip;                                                    \
            fkernel::ipc::SignalDelivery::send_signal(task, (signal), &si, true);         \
            return;                                                                      \
        }                                                                                \
    }                                                                                    \
                                                                                          \
    fk::algorithms::kexception(                                                         \
        exception_message,                                                              \
        "vector=%u error=%p RIP=%p RSP=%p RFLAGS=%p",                                     \
        (unsigned)vector, frame->error_code,                                               \
        frame->rip, frame->rsp, frame->rflags                                              \
    );                                                                                     \
    fk::algorithms::kexception(                                                         \
        exception_message,                                                              \
        "  rax=%p rbx=%p rcx=%p rdx=%p",                                                  \
        frame->rax, frame->rbx, frame->rcx, frame->rdx                                    \
    );                                                                                     \
    fk::algorithms::kexception(                                                         \
        exception_message,                                                              \
        "  rsi=%p rdi=%p rbp=%p r8=%p",                                                   \
        frame->rsi, frame->rdi, frame->rbp, frame->r8                                     \
    );                                                                                     \
    fk::algorithms::kexception(                                                         \
        exception_message,                                                              \
        "  r9=%p r10=%p r11=%p r12=%p r13=%p r14=%p r15=%p",                             \
        frame->r9, frame->r10, frame->r11,                                                 \
        frame->r12, frame->r13, frame->r14, frame->r15                                    \
    );                                                                                     \
                                                                                           \
    halt_forever();                                                                       \
}
