#pragma once

#include <Kernel/Arch/x86_64/Interrupt/Handler/interrupt_frame.h>
#include <LibC/stdint.h>

// === Exception Handler

void default_handler([[maybe_unused]] uint8_t vector,
                     InterruptFrame *frame = nullptr);
void nmi_handler([[maybe_unused]] uint8_t vector,
                 InterruptFrame *frame = nullptr);
void general_protection_handler([[maybe_unused]] uint8_t vector,
                                InterruptFrame *frame = nullptr);
void page_fault_handler([[maybe_unused]] uint8_t vector,
                        InterruptFrame *frame = nullptr);

// === Routines Handler

void timer_handler([[maybe_unused]] uint8_t vector,
                   InterruptFrame *frame = nullptr);
void apic_timer_handler([[maybe_unused]] uint8_t vector,
                   InterruptFrame *frame = nullptr);