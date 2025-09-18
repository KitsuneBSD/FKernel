#pragma once

#include <Kernel/Arch/x86_64/Interrupt/idt_types.h>

void default_handler([[maybe_unused]] InterruptFrame *frame,
                     [[maybe_unused]] uint8_t vector);
