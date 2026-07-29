#pragma once

#include <LibFK/Types/types.h>

// Set to 1 by arch_enable_cpu_features when XSAVE is available.
// context_switch.asm branches on this to use xsave64/xrstor64 instead of fxsave/fxrstor.
extern uint8_t g_use_xsave;
extern size_t  g_xsave_area_size;

extern "C" {

void arch_cpuid(uint32_t leaf, uint32_t subleaf,
                uint32_t* eax, uint32_t* ebx, uint32_t* ecx, uint32_t* edx);

void arch_write_msr(uint32_t msr, uint64_t value);
uint64_t arch_read_msr(uint32_t msr);

void arch_enable_cpu_features(bool has_smep, bool has_smap, bool has_nx,
                              bool has_xsave, bool has_avx,
                              bool has_fsgsbase, bool has_umip, bool has_invpcid);

[[noreturn]] void arch_halt_loop();

void arch_cpu_relax();

void arch_disable_interrupts();
void arch_enable_interrupts();

uint64_t arch_save_flags_and_disable();
void arch_restore_flags(uint64_t flags);

[[noreturn]] void arch_triple_fault();

void arch_smap_begin();
void arch_smap_end();

void detect_tsc_frequency();

uint64_t arch_read_tsc();
uint64_t arch_read_tsc_serialized(); // LFENCE + RDTSC

// CPU idle (sti + hlt — re-enables interrupts and halts until next interrupt)
void arch_cpu_idle();

// FPU context save/restore (uses XSAVE or FXSAVE depending on g_use_xsave)
void arch_fpu_save(void* area);
void arch_fpu_restore(const void* area);

// Memory — CR3 and TLB
void arch_write_cr3(void* pml4_virt_addr);
uintptr_t arch_read_cr3();
int arch_invlpg(uintptr_t addr);

// Segments — GDT and TSS
void arch_flush_gdt(void* gdtr);
void arch_flush_tss(uint16_t tss_selector);

// Interrupt — IDT
void arch_flush_idt(void* idtr);

} // extern "C"
