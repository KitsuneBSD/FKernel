#pragma once

#include <Kernel/Arch/x86_64/Interrupt/Handler/handlers.h>
#include <LibFK/Algorithms/log.h>
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
