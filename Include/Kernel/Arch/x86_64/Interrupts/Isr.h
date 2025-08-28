#pragma once

#include <LibC/stdint.h>

constexpr LibC::uint8_t IST_NONE = 0;
constexpr LibC::uint8_t IST_DOUBLE_FAULT = 1;
constexpr LibC::uint8_t IST_NMI = 2;

extern "C" void isr_divide_by_zero();
extern "C" void isr_debug();
extern "C" void isr_nmi();
extern "C" void isr_breakpoint();
extern "C" void isr_overflow();
extern "C" void isr_bound_range();
extern "C" void isr_invalid_opcode();
extern "C" void isr_device_na();
extern "C" void isr_double_fault();
extern "C" void isr_coprocessor_seg();
extern "C" void isr_invalid_tss();
extern "C" void isr_seg_not_present();
extern "C" void isr_stack_fault();
extern "C" void isr_gp_fault();
extern "C" void isr_page_fault();
extern "C" void isr_reserved_15();
extern "C" void isr_fpu_error();
extern "C" void isr_alignment_check();
extern "C" void isr_machine_check();
extern "C" void isr_simd_fp();
extern "C" void isr_virtualization();
extern "C" void isr_reserved_21();
extern "C" void isr_reserved_22();
extern "C" void isr_reserved_23();
extern "C" void isr_reserved_24();
extern "C" void isr_reserved_25();
extern "C" void isr_reserved_26();
extern "C" void isr_reserved_27();
extern "C" void isr_reserved_28();
extern "C" void isr_reserved_29();
extern "C" void isr_reserved_30();
extern "C" void isr_reserved_31();
