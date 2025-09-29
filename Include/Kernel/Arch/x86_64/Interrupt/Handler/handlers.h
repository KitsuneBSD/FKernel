#pragma once

#include <Kernel/Arch/x86_64/Interrupt/Handler/interrupt_frame.h>
#include <LibC/stdint.h>

void default_handler([[maybe_unused]] uint8_t vector,
                     InterruptFrame *frame = nullptr);
void nmi_handler([[maybe_unused]] uint8_t vector,
                 InterruptFrame *frame = nullptr);
void gp_handler([[maybe_unused]] uint8_t vector,
                InterruptFrame *frame = nullptr);

void timer_handler([[maybe_unused]] uint8_t vector,
                   InterruptFrame *frame = nullptr);
