#pragma once

#include <Kernel/Arch/x86_64/Interrupt/Handler/interrupt_frame.h>
#include <LibFK/Types/types.h>

void default_handler([[maybe_unused]] uint8_t vector,
                     InterruptFrame *frame = nullptr);

// Exceptions
void divide_by_zero_handler([[maybe_unused]] uint8_t vector, InterruptFrame *frame = nullptr);
void debug_handler([[maybe_unused]] uint8_t vector, InterruptFrame *frame = nullptr);
void nmi_handler([[maybe_unused]] uint8_t vector, InterruptFrame *frame = nullptr);
void breakpoint_handler([[maybe_unused]] uint8_t vector, InterruptFrame *frame = nullptr);
void overflow_handler([[maybe_unused]] uint8_t vector, InterruptFrame *frame = nullptr);
void bound_range_exceeded_handler([[maybe_unused]] uint8_t vector, InterruptFrame *frame = nullptr);
void invalid_opcode_handler([[maybe_unused]] uint8_t vector, InterruptFrame *frame = nullptr);
void device_not_available_handler([[maybe_unused]] uint8_t vector, InterruptFrame *frame = nullptr);
void double_fault_handler([[maybe_unused]] uint8_t vector, InterruptFrame *frame = nullptr);
void invalid_tss_handler([[maybe_unused]] uint8_t vector, InterruptFrame *frame = nullptr);
void segment_not_present_handler([[maybe_unused]] uint8_t vector, InterruptFrame *frame = nullptr);
void stack_segment_fault_handler([[maybe_unused]] uint8_t vector, InterruptFrame *frame = nullptr);
void general_protection_handler([[maybe_unused]] uint8_t vector, InterruptFrame *frame = nullptr);
void page_fault_handler([[maybe_unused]] uint8_t vector, InterruptFrame *frame = nullptr);
void x87_fpu_floating_point_error_handler([[maybe_unused]] uint8_t vector, InterruptFrame *frame = nullptr);
void alignment_check_handler([[maybe_unused]] uint8_t vector, InterruptFrame *frame = nullptr);
void machine_check_handler([[maybe_unused]] uint8_t vector, InterruptFrame *frame = nullptr);
void simd_floating_point_exception_handler([[maybe_unused]] uint8_t vector, InterruptFrame *frame = nullptr);
void virtualization_exception_handler([[maybe_unused]] uint8_t vector, InterruptFrame *frame = nullptr);

// IRQs
void timer_handler([[maybe_unused]] uint8_t vector,
                   InterruptFrame *frame = nullptr);

void keyboard_handler([[maybe_unused]] uint8_t vector,
                      InterruptFrame *frame = nullptr);

void ata_primary_handler([[maybe_unused]] uint8_t vector,
                         InterruptFrame *frame = nullptr);
void ata_secondary_handler([[maybe_unused]] uint8_t vector,
                           InterruptFrame *frame = nullptr);

void apic_timer_handler([[maybe_unused]] uint8_t vector,
                        InterruptFrame *frame = nullptr);
