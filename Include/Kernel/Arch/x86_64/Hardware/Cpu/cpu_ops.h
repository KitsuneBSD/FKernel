#pragma once

#include <LibFK/Types/types.h>

extern "C" {

void arch_cpuid(uint32_t leaf, uint32_t subleaf,
                uint32_t* eax, uint32_t* ebx, uint32_t* ecx, uint32_t* edx);

void arch_write_msr(uint32_t msr, uint64_t value);
uint64_t arch_read_msr(uint32_t msr);

void arch_enable_cpu_features(bool has_smep, bool has_smap, bool has_nx);

[[noreturn]] void arch_halt_loop();

void arch_cpu_relax();

void arch_disable_interrupts();
void arch_enable_interrupts();

uint64_t arch_save_flags_and_disable();
void arch_restore_flags(uint64_t flags);

[[noreturn]] void arch_triple_fault();

void arch_smap_begin();
void arch_smap_end();

} // extern "C"
