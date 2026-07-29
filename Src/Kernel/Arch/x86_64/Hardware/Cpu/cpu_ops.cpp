#include <Kernel/Arch/x86_64/Hardware/Cpu/cpu_ops.h>
#include <Kernel/Arch/x86_64/Syscall/syscall_arch.h>
#include <Kernel/Boot/boot_timer.h>
#include <Kernel/Hardware/Cpu/cpu.h>
#include <LibFK/Algorithms/log.h>

size_t g_xsave_area_size = 512;  // default: FXSAVE size, updated in arch_enable_cpu_features
uint8_t g_use_xsave = 0;         // 1 when XSAVE/XRSTOR should be used in context switch
uint8_t g_has_ermsb = 0;         // 1 when ERMSB (Enhanced REP MOVSB/STOSB) is available

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

static void enable_fast_strings_ermsb() {
  constexpr uint32_t MSR_IA32_MISC_ENABLE = 0x1A0;
  uint32_t lo, hi;
  asm volatile("rdmsr" : "=a"(lo), "=d"(hi) : "c"(MSR_IA32_MISC_ENABLE));
  if (!(lo & 1u)) {
    lo |= 1u; // bit 0: Fast Strings Enable
    asm volatile("wrmsr" :: "c"(MSR_IA32_MISC_ENABLE), "a"(lo), "d"(hi));
    fk::algorithms::klog("CPU", "Fast Strings enabled via IA32_MISC_ENABLE");
  }
  // ERMSB: CPUID leaf 7, EBX bit 9 — detect only, no MSR write
  uint32_t eax, ebx, ecx, edx;
  arch_cpuid(7, 0, &eax, &ebx, &ecx, &edx);
  if (ebx & (1u << 9)) {
    g_has_ermsb = 1;
    fk::algorithms::klog("CPU", "ERMSB supported");
  }
}

extern "C" void arch_enable_cpu_features(bool has_smep, bool has_smap, bool has_nx,
                                         bool has_xsave, bool has_avx,
                                         bool has_fsgsbase, bool has_umip, bool has_invpcid) {
  fk::algorithms::klog("CPU", "Initializing features (SSE, NX, PCID)...");

  // Enable Fast Strings and detect ERMSB before touching CR0/CR4
  enable_fast_strings_ermsb();

  uint64_t cr0, cr4;
  asm volatile("mov %%cr0, %0" : "=r"(cr0));
  cr0 &= ~(1ULL << 2); // Clear EM (Emulation)
  cr0 |= (1ULL << 1);  // Set MP (Monitor Coprocessor)
  cr0 |= (1ULL << 16); // Set WP (Write Protect) — required for CoW correctness
  asm volatile("mov %0, %%cr0" ::"r"(cr0));

  asm volatile("mov %%cr4, %0" : "=r"(cr4));
  cr4 |= (1ULL << 9);  // OSFXSR: FXSAVE/FXRSTOR support
  cr4 |= (1ULL << 10); // OSXMMEXCPT: SIMD exception support
  if (has_smep)
    cr4 |= (1ULL << 20); // SMEP: prevent kernel executing user-space pages
  if (has_smap)
    cr4 |= (1ULL << 21); // SMAP: prevent kernel accessing user-space pages directly
  if (has_fsgsbase)
    cr4 |= (1ULL << 16); // FSGSBASE: enable RDFSBASE/WRFSBASE/RDGSBASE/WRGSBASE instructions
  if (has_umip)
    cr4 |= (1ULL << 11); // UMIP: prevent ring-3 from executing SGDT/SIDT/SLDT/SMSW/STR
  if (has_xsave)
    cr4 |= (1ULL << 18); // OSXSAVE: enable XSAVE/XRSTOR instructions
  // PCID: CPUID leaf 1, ECX bit 17
  {
    uint32_t c1_eax, c1_ebx, c1_ecx, c1_edx;
    arch_cpuid(1, 0, &c1_eax, &c1_ebx, &c1_ecx, &c1_edx);
    if (c1_ecx & (1u << 17)) {
      cr4 |= (1ULL << 17); // CR4.PCIDE: enable Process-Context Identifiers
      fk::algorithms::klog("CPU", "PCID enabled (CR4.PCIDE=1)");
    }
  }
  asm volatile("mov %0, %%cr4" ::"r"(cr4));

  if (has_fsgsbase)
    fk::algorithms::klog("CPU", "FSGSBASE enabled (CR4.FSGSBASE=1)");
  if (has_umip)
    fk::algorithms::klog("CPU", "UMIP enabled (CR4.UMIP=1)");
  if (has_invpcid)
    fk::algorithms::klog("CPU", "INVPCID available");

  if (has_nx) {
    uint64_t efer = arch_read_msr(MSR_EFER);
    arch_write_msr(MSR_EFER, efer | EFER_NXE);
  }

  if (has_xsave) {
    uint64_t xcr0 = 0;
    xcr0 |= (1ULL << 0);  // x87 state
    xcr0 |= (1ULL << 1);  // SSE state
    if (has_avx)
      xcr0 |= (1ULL << 2); // AVX state
    uint32_t low = xcr0 & 0xFFFFFFFF;
    uint32_t high = xcr0 >> 32;
    asm volatile("xsetbv" : : "a"(low), "d"(high), "c"(0) : "memory");
    fk::algorithms::klog("CPU", "XSAVE enabled (xcr0=0x%lx)", xcr0);

    // Detect XSAVE area size via CPUID 0x0D, sub-leaf 0
    uint32_t xsave_eax, xsave_ebx, xsave_ecx, xsave_edx;
    asm volatile("cpuid"
                 : "=a"(xsave_eax), "=b"(xsave_ebx), "=c"(xsave_ecx), "=d"(xsave_edx)
                 : "a"(0x0D), "c"(0));
    g_xsave_area_size = xsave_ebx > 512 ? xsave_ebx : 512;
    fk::algorithms::klog("CPU", "XSAVE area size: %zu bytes (EBX=%u)", g_xsave_area_size, xsave_ebx);
    g_use_xsave = 1;
  }
}

extern "C" [[noreturn]] void arch_halt_loop() {
  for (;;)
    asm volatile("hlt");
}

extern "C" void arch_cpu_idle() {
  asm volatile("sti; hlt");
}

extern "C" void arch_fpu_save(void* area) {
  if (g_use_xsave) {
    uint32_t mask_lo = 0xFFFFFFFFu, mask_hi = 0xFFFFFFFFu;
    asm volatile("xsave64 %0" : "=m"(*static_cast<uint8_t*>(area))
                 : "a"(mask_lo), "d"(mask_hi) : "memory");
  } else {
    asm volatile("fxsave %0" : "=m"(*static_cast<uint8_t*>(area)) :: "memory");
  }
}

extern "C" void arch_fpu_restore(const void* area) {
  if (g_use_xsave) {
    uint32_t mask_lo = 0xFFFFFFFFu, mask_hi = 0xFFFFFFFFu;
    asm volatile("xrstor64 %0" :: "m"(*static_cast<const uint8_t*>(area)),
                 "a"(mask_lo), "d"(mask_hi) : "memory");
  } else {
    asm volatile("fxrstor %0" :: "m"(*static_cast<const uint8_t*>(area)) : "memory");
  }
}

extern "C" uint64_t arch_read_tsc() {
  uint32_t lo, hi;
  asm volatile("rdtsc" : "=a"(lo), "=d"(hi));
  return (static_cast<uint64_t>(hi) << 32) | lo;
}

extern "C" uint64_t arch_read_tsc_serialized() {
  uint32_t lo, hi;
  asm volatile("lfence; rdtsc" : "=a"(lo), "=d"(hi));
  return (static_cast<uint64_t>(hi) << 32) | lo;
}

void detect_tsc_frequency() {
  uint32_t eax, ebx, ecx, edx;

  // CPUID 0x15: Time Stamp Counter / Core Crystal Clock Information
  arch_cpuid(0x15, 0, &eax, &ebx, &ecx, &edx);
  if (ecx != 0 && eax != 0) {
    uint64_t crystal_hz = ecx;
    uint64_t freq = crystal_hz * ebx / eax;
    BootTimer::the().set_tsc_frequency(freq);
    fk::algorithms::klog("CPU", "TSC frequency: %llu Hz (CPUID 0x15)", freq);
    return;
  }

  // CPUID 0x16: Processor Frequency Information
  arch_cpuid(0x16, 0, &eax, &ebx, &ecx, &edx);
  if (eax > 0) {
    uint64_t freq = static_cast<uint64_t>(eax) * 1000000ULL;
    BootTimer::the().set_tsc_frequency(freq);
    fk::algorithms::klog("CPU", "TSC frequency: %llu Hz (CPUID 0x16, base=%u MHz)", freq, eax);
    return;
  }

  fk::algorithms::kwarn("CPU", "TSC frequency not available from CPUID, boot timer in cycles");
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
  if (CPU::the().has_smap())
    asm volatile("stac" ::: "memory");
}

extern "C" void arch_smap_end() {
  if (CPU::the().has_smap())
    asm volatile("clac" ::: "memory");
}
