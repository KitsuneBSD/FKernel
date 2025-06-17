#pragma once

#include <LibC/stdint.h>

#define LAPIC_BASE 0xFEE00000
#define LAPIC_SVR 0xF0
#define LAPIC_ENABLE 0x100

inline void cpuid(uint32_t leaf, uint32_t &eax, uint32_t &ebx, uint32_t &ecx,
                  uint32_t &edx) {
  asm volatile("cpuid"
               : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx)
               : "a"(leaf));
}

bool cpu_has_apic();
void enable_lapic();
void init_apic();
