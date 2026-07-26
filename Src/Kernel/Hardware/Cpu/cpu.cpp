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

  // Check for APIC
  cpuid(1, 0, &eax, &ebx, &ecx, &edx);
  if (edx & (1 << 9)) {
    fk::algorithms::kdebug("CPU", "Found APIC support");
    m_has_apic = true;
  }
  m_lapic_id = static_cast<uint8_t>(ebx >> 24);

  // Check for x2APIC
  if (ecx & (1 << 21)) {
    fk::algorithms::kdebug("CPU", "Found x2APIC support");
    m_has_x2apic = true;
  }

  // Check for hpet
  if (ACPIManager::the().find_table("HPET")) {
    fk::algorithms::kdebug("CPU", "Found HPET support");
    m_has_hpet = true;
  }

  // Check for NX support (CPUID 0x80000001, EDX bit 20)
  cpuid(0x80000001, 0, &eax, &ebx, &ecx, &edx);
  if (edx & (1 << 20))
    m_has_nx = true;

  // Check for SMEP (bit 7) and SMAP (bit 20) via CPUID leaf 7, subleaf 0
  cpuid(7, 0, &eax, &ebx, &ecx, &edx);
  if (ebx & (1 << 7))
    m_has_smep = true;
  if (ebx & (1 << 20))
    m_has_smap = true;
}

void CPU::initialize_features() {
  arch_enable_cpu_features(m_has_smep, m_has_smap, m_has_nx);
}

void CPU::write_msr(uint32_t msr, uint64_t value) {
  arch_write_msr(msr, value);
}

uint64_t CPU::read_msr(uint32_t msr) {
  return arch_read_msr(msr);
}
