#include <Kernel/Hardware/Acpi/acpi.h>
#include <Kernel/Hardware/Cpu/cpu.h>
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
  asm volatile("cpuid"
               : "=a"(*a), "=b"(*b), "=c"(*c), "=d"(*d)
               : "a"(eax), "c"(ecx));
}

void CPU::detect_cpu_features() {
  uint32_t eax, ebx, ecx, edx;

  // Check for APIC
  cpuid(1, 0, &eax, &ebx, &ecx, &edx);
  if (edx & (1 << 9)) {
    /*TODO: Apply this log when we work with LogLevel
    fk::algorithms::kdebug("CPU", "Found APIC support");
    */
    m_has_apic = true;
  }

  // Check for x2APIC
  if (ecx & (1 << 21)) {
    /*TODO: Apply this log when we work with LogLevel
    fk::algorithms::kdebug("CPU", "Found x2APIC support");
    */
    m_has_x2apic = true;
  }

  // Check for hpet
  if (ACPIManager::the().find_table("HPET")) {
    /*TODO: Apply this log when we work with LogLevel
    fk::algorithms::kdebug("CPU", "Found Hpet support");
    */
    m_has_hpet = true;
  }
}

void CPU::write_msr(uint32_t msr, uint64_t value) {
  uint32_t low = value & 0xFFFFFFFF;
  uint32_t high = value >> 32;
  asm volatile("wrmsr" : : "c"(msr), "a"(low), "d"(high));
  /*TODO: Apply this log when we work with LogLevel
  fk::algorithms::kdebug("CPU", "Wrote MSR %lx = %lx", msr, value);
  */
}

uint64_t CPU::read_msr(uint32_t msr) {
  uint32_t low, high;
  asm volatile("rdmsr" : "=a"(low), "=d"(high) : "c"(msr));
  uint64_t value = ((uint64_t)high << 32) | low;
  /*TODO: Apply this log when we work with LogLevel
  fk::algorithms::kdebug("CPU", "Read MSR %lx = %lx", msr, value);
  */
  return value;
}
