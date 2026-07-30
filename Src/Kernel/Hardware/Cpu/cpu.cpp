#include <Kernel/Hardware/Acpi/acpi.h>
#include <Kernel/Hardware/Cpu/cpu.h>
#include <Kernel/Arch/x86_64/Hardware/Cpu/cpu_ops.h>
#include <LibFK/Algorithms/log.h>

CPU::CPU() {
  uint32_t eax, ebx, ecx, edx;

  // Get vendor ID
  cpuid(0, 0, &eax, &ebx, &ecx, &edx);
  char vendor[13];
  *(uint32_t *)&vendor[0] = ebx;
  *(uint32_t *)&vendor[4] = edx;
  *(uint32_t *)&vendor[8] = ecx;
  vendor[12] = '\0';
  m_vendor = fk::text::String(vendor);

  cpuid(0x80000000, 0, &eax, &ebx, &ecx, &edx);
  if (eax >= 0x80000004) {
    char brand[49];
    cpuid(0x80000002, 0, (uint32_t *)&brand[0], (uint32_t *)&brand[4],
          (uint32_t *)&brand[8], (uint32_t *)&brand[12]);
    cpuid(0x80000003, 0, (uint32_t *)&brand[16], (uint32_t *)&brand[20],
          (uint32_t *)&brand[24], (uint32_t *)&brand[28]);
    cpuid(0x80000004, 0, (uint32_t *)&brand[32], (uint32_t *)&brand[36],
          (uint32_t *)&brand[40], (uint32_t *)&brand[44]);
    brand[48] = '\0';
    m_brand = fk::text::String(brand);
  } else {
    m_brand = fk::text::String("Unknown");
  }

  detect_cpu_features();
}

void CPU::cpuid(uint32_t eax, uint32_t ecx, uint32_t *a, uint32_t *b,
                uint32_t *c, uint32_t *d) {
  arch_cpuid(eax, ecx, a, b, c, d);
}

void CPU::detect_cpu_features() {
  uint32_t eax, ebx, ecx, edx;

  // Leaf 1: basic feature flags
  cpuid(1, 0, &eax, &ebx, &ecx, &edx);
  m_lapic_id = static_cast<uint8_t>(ebx >> 24);
  if (edx & (1u << 9)) {
    fk::algorithms::kdebug("CPU", "Found APIC support");
    m_has_apic = true;
  }
  if (ecx & (1u << 21)) {
    fk::algorithms::kdebug("CPU", "Found x2APIC support");
    m_has_x2apic = true;
  }
  // XSAVE: leaf 1, ECX bit 26; AVX: leaf 1, ECX bit 28
  m_has_xsave = (ecx & (1u << 26)) != 0;
  m_has_avx   = (ecx & (1u << 28)) != 0;

  // HPET via ACPI table
  if (ACPIManager::the().find_table("HPET")) {
    fk::algorithms::kdebug("CPU", "Found HPET support");
    m_has_hpet = true;
  }

  // Leaf 0x80000001: extended feature flags
  cpuid(0x80000001, 0, &eax, &ebx, &ecx, &edx);
  m_has_nx         = (edx & (1u << 20)) != 0;
  m_has_1gb_pages  = (edx & (1u << 26)) != 0;

  // Physical/virtual address widths
  cpuid(0x80000008, 0, &eax, &ebx, &ecx, &edx);
  m_phys_addr_width = static_cast<uint8_t>(eax & 0xFF);
  m_virt_addr_width = static_cast<uint8_t>((eax >> 8) & 0xFF);
  if (!m_phys_addr_width) m_phys_addr_width = 36;
  if (!m_virt_addr_width) m_virt_addr_width = 48;
  fk::algorithms::klog("CPU", "Address widths: phys=%u virt=%u", m_phys_addr_width, m_virt_addr_width);

  // Leaf 7, subleaf 0: structured feature flags
  cpuid(7, 0, &eax, &ebx, &ecx, &edx);
  m_has_fsgsbase = (ebx & (1u << 0)) != 0;
  m_has_smep     = (ebx & (1u << 7)) != 0;
  m_has_invpcid  = (ebx & (1u << 10)) != 0;
  m_has_avx2     = (ebx & (1u << 5)) != 0;
  m_has_avx512   = (ebx & (1u << 16)) != 0;  // AVX-512F
  m_has_smap     = (ebx & (1u << 20)) != 0;
  m_has_umip     = (ecx & (1u << 2)) != 0;
  m_has_la57     = (ecx & (1u << 16)) != 0;
  m_has_cet      = (ecx & (1u << 7)) != 0;   // CET shadow stack (user)

  fk::algorithms::klog("CPU", "Features: SMEP=%d SMAP=%d NX=%d XSAVE=%d AVX=%d AVX2=%d AVX512=%d",
    m_has_smep, m_has_smap, m_has_nx, m_has_xsave, m_has_avx, m_has_avx2, m_has_avx512);
  fk::algorithms::klog("CPU", "Features: FSGSBASE=%d UMIP=%d INVPCID=%d 1GB=%d LA57=%d CET=%d",
    m_has_fsgsbase, m_has_umip, m_has_invpcid, m_has_1gb_pages, m_has_la57, m_has_cet);
}

void CPU::initialize_features() {
  arch_enable_cpu_features(m_has_smep, m_has_smap, m_has_nx, m_has_xsave, m_has_avx,
                           m_has_fsgsbase, m_has_umip, m_has_invpcid);
}

void CPU::write_msr(uint32_t msr, uint64_t value) {
  arch_write_msr(msr, value);
}

uint64_t CPU::read_msr(uint32_t msr) {
  return arch_read_msr(msr);
}
