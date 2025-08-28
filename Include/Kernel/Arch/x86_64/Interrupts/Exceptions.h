#pragma once

#include <Kernel/Arch/x86_64/Cpu/State.h>
#include <LibC/stdint.h>

extern void (*exception_stubs[32])();
extern LibC::uint8_t const isr_ist[32];

void halt(void);

void global_default_handler(struct CpuState const* frame);

// === Specific Custom Handle
void divide_by_zero_handler(struct CpuState* frame);
void general_protection_fault_handler(struct CpuState* frame);

char const* named_exception(int index) noexcept;
