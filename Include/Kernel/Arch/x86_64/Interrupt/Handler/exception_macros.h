#pragma once

#include <Kernel/Arch/x86_64/Interrupt/Handler/handlers.h>
#include "LibFK/Algorithms/log.h"
#include <LibFK/Types/types.h>

#define GENERIC_EXCEPTION_HANDLER(name, exception_message)                     \
  void name([[maybe_unused]] uint8_t vector, InterruptFrame *frame) {          \
    fk::algorithms::kexception(exception_message, "Exception: vector=%u", (unsigned)vector);   \
    fk::algorithms::kexception(exception_message, "RIP=%p CS=%p RFLAGS=%p", frame->rip,         \
               frame->cs, frame->rflags);                                      \
    fk::algorithms::kexception(exception_message, "RSP=%p SS=%p", frame->rsp, frame->ss);       \
    fk::algorithms::kexception(exception_message, "RAX=%p RBX=%p RCX=%p RDX=%p", frame->rax,    \
               frame->rbx, frame->rcx, frame->rdx);                            \
    fk::algorithms::kexception(exception_message, "RSI=%p RDI=%p RBP=%p", frame->rsi,           \
               frame->rdi, frame->rbp);                                        \
    fk::algorithms::kexception(exception_message, "R8=%p R9=%p R10=%p R11=%p", frame->r8,       \
               frame->r9, frame->r10, frame->r11);                             \
    fk::algorithms::kexception(exception_message, "R12=%p R13=%p R14=%p R15=%p", frame->r12,    \
               frame->r13, frame->r14, frame->r15);                            \
    for (;;) {                                                                 \
      asm volatile("cli;hlt");                                                 \
    }                                                                          \
  }

#define GENERIC_EXCEPTION_HANDLER_WITH_ERROR_CODE(name, exception_message)     \
  void name([[maybe_unused]] uint8_t vector, InterruptFrame *frame) {          \
    fk::algorithms::kexception(exception_message, "Exception: vector=%u, error_code=%p",       \
               (unsigned)vector, frame->error_code);                           \
    fk::algorithms::kexception(exception_message, "RIP=%p CS=%p RFLAGS=%p", frame->rip,         \
               frame->cs, frame->rflags);                                      \
    fk::algorithms::kexception(exception_message, "RSP=%p SS=%p", frame->rsp, frame->ss);       \
    fk::algorithms::kexception(exception_message, "RAX=%p RBX=%p RCX=%p RDX=%p", frame->rax,    \
               frame->rbx, frame->rcx, frame->rdx);                            \
    fk::algorithms::kexception(exception_message, "RSI=%p RDI=%p RBP=%p", frame->rsi,           \
               frame->rdi, frame->rbp);                                        \
    fk::algorithms::kexception(exception_message, "R8=%p R9=%p R10=%p R11=%p", frame->r8,       \
               frame->r9, frame->r10, frame->r11);                             \
    fk::algorithms::kexception(exception_message, "R12=%p R13=%p R14=%p R15=%p", frame->r12,    \
               frame->r13, frame->r14, frame->r15);                            \
    for (;;) {                                                                 \
      asm volatile("cli;hlt");                                                 \
    }                                                                          \
  }


