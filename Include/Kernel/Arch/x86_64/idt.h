#pragma once

#include <Arch/x86_64/isr.h>
#include <Arch/x86_64/stack_size.h>
#include <LibC/stddef.h>
#include <LibC/stdint.h>

struct IDTEntry {
  uint16_t offset_low;
  uint16_t selector;
  uint8_t ist;
  uint8_t type_attr;
  uint16_t offset_middle;
  uint32_t offset_high;
  uint32_t zero;
} __attribute__((packed));

static_assert(sizeof(IDTEntry) == 16, "IDTEntry must be 16 bytes");

struct IDTPointer {
  uint16_t limit;
  uint64_t base;
} __attribute__((packed));

static_assert(sizeof(IDTPointer) == 10, "IDTPointer must be 10 bytes");

constexpr uint8_t IDT_INTERRUPT_GATE_FLAGS = 0x8E;
constexpr uint8_t IST_NONE = 0;
constexpr uint8_t IST_DOUBLE_FAULT = 1;

static constexpr void (*isr_handlers[IDT_ENTRIES])() = {
    isr_divide_by_zero,  isr_debug,       isr_nmi,
    isr_breakpoint,      isr_overflow,    isr_bound_range,
    isr_invalid_opcode,  isr_device_na,   isr_double_fault,
    isr_coprocessor_seg, isr_invalid_tss, isr_seg_not_present,
    isr_stack_fault,     isr_gp_fault,    isr_page_fault,
    isr_reserved_15,     isr_fpu_error,   isr_alignment_check,
    isr_machine_check,   isr_simd_fp,     isr_virtualization,
    isr_reserved_21,     isr_reserved_22, isr_reserved_23,
    isr_reserved_24,     isr_reserved_25, isr_reserved_26,
    isr_reserved_27,     isr_reserved_28, isr_reserved_29,
    isr_reserved_30,     isr_reserved_31};

static constexpr uint8_t isr_ist[IDT_ENTRIES] = {
    IST_NONE, IST_NONE, IST_NONE,         IST_NONE, IST_NONE, IST_NONE,
    IST_NONE, IST_NONE, IST_DOUBLE_FAULT, IST_NONE, IST_NONE, IST_NONE,
    IST_NONE, IST_NONE, IST_NONE,         IST_NONE, IST_NONE, IST_NONE,
    IST_NONE, IST_NONE, IST_NONE,         IST_NONE, IST_NONE, IST_NONE,
    IST_NONE, IST_NONE, IST_NONE,         IST_NONE, IST_NONE, IST_NONE,
    IST_NONE, IST_NONE,
};

extern "C" void flush_idt(IDTPointer *idtr);

void init_idt();
