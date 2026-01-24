#pragma once

#include <Kernel/Arch/x86_64/Interrupt/Handler/handlers.h>
#include <LibFK/Algorithms/log.h>
#include <LibFK/Core/Assertions.h>
#include <LibFK/Types/types.h>

[[noreturn]] inline void halt_forever() {
    for (;;) {
        asm volatile("cli; hlt");
    }
}

#define GENERIC_EXCEPTION_HANDLER(name, exception_message)            \
void name(uint8_t vector, InterruptFrame* frame) {                   \
    assert(frame && "Exception without interrupt frame");            \
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
    assert(frame && "Exception without interrupt frame");                  \
                                                                             \
    fk::algorithms::kexception(                                            \
        exception_message,                                                 \
        "vector=%u error=%p RIP=%p RSP=%p RFLAGS=%p",                      \
        (unsigned)vector, frame->error_code,                                \
        frame->rip, frame->rsp, frame->rflags                               \
    );                                                                       \
                                                                             \
    halt_forever();                                                          \
}
