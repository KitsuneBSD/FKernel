#include <Kernel/Arch/x86_64/Hardware/Cpu/cpu_ops.h>
#include <Kernel/Arch/x86_64/Syscall/syscall_arch.h>
#include <LibFK/Algorithms/log.h>

extern "C" void arch_cpuid(uint32_t leaf, uint32_t subleaf,
                            uint32_t* eax, uint32_t* ebx, uint32_t* ecx, uint32_t* edx) {
  asm volatile("cpuid"
               : "=a"(*eax), "=b"(*ebx), "=c"(*ecx), "=d"(*edx)
               : "a"(leaf), "c"(subleaf));
}

extern "C" void arch_write_msr(uint32_t msr, uint64_t value) {
  uint32_t low = value & 0xFFFFFFFF;
  uint32_t high = value >> 32;
  asm volatile("wrmsr" : : "c"(msr), "a"(low), "d"(high));
}

extern "C" uint64_t arch_read_msr(uint32_t msr) {
  uint32_t low, high;
  asm volatile("rdmsr" : "=a"(low), "=d"(high) : "c"(msr));
  return ((uint64_t)high << 32) | low;
}

extern "C" void arch_enable_cpu_features(bool has_smep, bool has_smap, bool has_nx) {
  fk::algorithms::klog("CPU", "Initializing features (SSE, NX)...");

  uint64_t cr0, cr4;
  asm volatile("mov %%cr0, %0" : "=r"(cr0));
  cr0 &= ~(1ULL << 2); // Clear EM (Emulation)
  cr0 |= (1ULL << 1);  // Set MP (Monitor Coprocessor)
  asm volatile("mov %0, %%cr0" ::"r"(cr0));

  asm volatile("mov %%cr4, %0" : "=r"(cr4));
  cr4 |= (1ULL << 9);  // OSFXSR: FXSAVE/FXRSTOR support
  cr4 |= (1ULL << 10); // OSXMMEXCPT: SIMD exception support
  if (has_smep)
    cr4 |= (1ULL << 20); // SMEP: prevent kernel executing user-space pages
  if (has_smap)
    cr4 |= (1ULL << 21); // SMAP: prevent kernel accessing user-space pages directly
  asm volatile("mov %0, %%cr4" ::"r"(cr4));

  if (has_nx) {
    uint64_t efer = arch_read_msr(MSR_EFER);
    arch_write_msr(MSR_EFER, efer | EFER_NXE);
  }
}

extern "C" [[noreturn]] void arch_halt_loop() {
  for (;;)
    asm volatile("hlt");
}

extern "C" void arch_cpu_relax() {
  asm volatile("pause" ::: "memory");
}

extern "C" void arch_disable_interrupts() {
  asm volatile("cli" ::: "memory");
}

extern "C" void arch_enable_interrupts() {
  asm volatile("sti" ::: "memory");
}

extern "C" uint64_t arch_save_flags_and_disable() {
  uint64_t flags;
  asm volatile("pushfq ; popq %0 ; cli" : "=r"(flags) : : "memory");
  return flags;
}

extern "C" void arch_restore_flags(uint64_t flags) {
  asm volatile("pushq %0 ; popfq" : : "g"(flags) : "memory");
}

extern "C" [[noreturn]] void arch_triple_fault() {
  asm volatile("lidt 0; int $3" : : : "memory");
  for (;;)
    asm volatile("hlt");
}

extern "C" void arch_smap_begin() {
  asm volatile("stac" ::: "memory");
}

extern "C" void arch_smap_end() {
  asm volatile("clac" ::: "memory");
}
