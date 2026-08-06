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

// Unified terminal for unrecoverable exceptions: banner + CPU ID + vector,
// full register dump, stack trace, then kfatal (halt). Defined in
// Src/Kernel/Arch/x86_64/Panic/panic.cpp.
extern "C" void panic_exception(uint8_t vector, InterruptFrame* frame, const char* message);

#define GENERIC_EXCEPTION_HANDLER(name, exception_message)            \
void name(uint8_t vector, InterruptFrame* frame) {                   \
    if (!frame) halt_forever();                                        \
    panic_exception(vector, frame, exception_message);                 \
}

#define GENERIC_EXCEPTION_HANDLER_WITH_ERROR_CODE(name, exception_message) \
void name(uint8_t vector, InterruptFrame* frame) {                         \
    if (!frame) halt_forever();                                              \
    panic_exception(vector, frame, exception_message);                       \
}

#define GENERIC_EXCEPTION_HANDLER_USER(name, exception_message, signal, si_code_val) \
void name(uint8_t vector, InterruptFrame* frame) {                                    \
    if (!frame) halt_forever();                                                         \
    auto* _exc_task = SchedulerManager::the().current();                                \
    if ((frame->cs & 3) == 3) {                                                         \
        if (_exc_task && !_exc_task->is_a_kernel_task()) {                              \
            siginfo_t si{};                                                              \
            si.si_signo = (signal);                                                      \
            si.si_code  = (si_code_val);                                                 \
            si.si_addr  = frame->rip;                                                    \
            fkernel::ipc::SignalDelivery::send_signal(_exc_task, (signal), &si, true);   \
            return;                                                                      \
        }                                                                                \
    } else if (_exc_task && !_exc_task->is_a_kernel_task()) {                           \
        fk::algorithms::kwarn(exception_message,                                      \
            "kernel-mode fault RIP=%p pid=%lu, killing user task",                      \
            (void*)frame->rip, _exc_task->control.identity.id.value());                 \
        SchedulerManager::the().kill_current_from_exception(signal);                    \
    }                                                                                    \
    panic_exception(vector, frame, exception_message);                                   \
}

#define GENERIC_EXCEPTION_HANDLER_USER_WITH_ERROR(name, exception_message, signal, si_code_val) \
void name(uint8_t vector, InterruptFrame* frame) {                                      \
    if (!frame) halt_forever();                                                           \
    auto* _exc_task = SchedulerManager::the().current();                                  \
    if ((frame->cs & 3) == 3) {                                                           \
        if (_exc_task && !_exc_task->is_a_kernel_task()) {                                \
            siginfo_t si{};                                                                \
            si.si_signo = (signal);                                                        \
            si.si_code  = (si_code_val);                                                   \
            si.si_addr  = frame->rip;                                                      \
            fkernel::ipc::SignalDelivery::send_signal(_exc_task, (signal), &si, true);    \
            return;                                                                        \
        }                                                                                  \
    } else if (_exc_task && !_exc_task->is_a_kernel_task()) {                             \
        fk::algorithms::kwarn(exception_message,                                        \
            "kernel-mode fault RIP=%p pid=%lu err=%p, killing user task",                 \
            (void*)frame->rip, _exc_task->control.identity.id.value(),                   \
            (void*)frame->error_code);                                                    \
        SchedulerManager::the().kill_current_from_exception(signal);                      \
    }                                                                                    \
    panic_exception(vector, frame, exception_message);                                   \
}
