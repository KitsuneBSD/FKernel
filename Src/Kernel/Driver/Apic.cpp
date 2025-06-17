#include <Driver/Apic.h>
#include <Driver/IOApic.h>
#include <Driver/Pic.h>
#include <LibFK/Log.h>

volatile uint32_t *lapic = (volatile uint32_t *)LAPIC_BASE;

bool cpu_has_apic() {
  uint32_t eax, ebx, ecx, edx;
  cpuid(1, eax, ebx, ecx, edx);
  return edx & (1 << 9);
}

void enable_lapic() { lapic[LAPIC_SVR / 4] = LAPIC_ENABLE | 0xFF; }

void init_apic() {
  disable_pic();

  if (!cpu_has_apic())
    Logf(LogLevel::ERROR, "APIC not supported");

  enable_lapic();

  for (uint8_t irq = 0; irq < 16; ++irq) {
    ioapic_redirect_irq(irq, 0x20 + irq);
  }

  Log(LogLevel::INFO, "APIC and IOAPIC initialized.");
}
