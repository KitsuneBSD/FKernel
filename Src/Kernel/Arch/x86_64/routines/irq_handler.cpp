#include <Arch/x86_64/irq_handler.h>
#include <Driver/Pic.h>

static volatile uint64_t tick_count = 0;

extern "C" void timer_handler(void *context) {
  ++tick_count;

  send_eoi(0);
}

extern "C" void keyboard_handler(void *context) {
  uint8_t scancode = inb(0x60);
#ifdef FKERNEL_DEBUG
  Logf(LogLevel::TRACE, "Keyboard IRQ - scancode: 0x%02X", scancode);
#endif

  send_eoi(1);
}

extern "C" void cascade_handler(void *context) {
#ifdef FKERNEL_DEBUG
  Logf(LogLevel::TRACE, "Cascade IRQ2 triggered.");
#endif

  send_eoi(2);
}

extern "C" void com2_handler(void *context) {
#ifdef FKERNEL_DEBUG
  Logf(LogLevel::TRACE, "Serial COM2 IRQ3 triggered.");
#endif
  send_eoi(3);
}

extern "C" void com1_handler(void *context) {
#ifdef FKERNEL_DEBUG
  Logf(LogLevel::TRACE, "Serial COM1 IRQ4 triggered.");
#endif
  send_eoi(4);
}

extern "C" void legacy_peripheral_handler(void *context) {
#ifdef FKERNEL_DEBUG
  Logf(LogLevel::TRACE, "Legacy peripheral IRQ5 triggered.");
#endif
  send_eoi(5);
}

extern "C" void fdc_handler(void *context) {
#ifdef FKERNEL_DEBUG
  Logf(LogLevel::TRACE, "Floppy Disk Controller IRQ6 triggered.");
#endif
  send_eoi(6);
}

extern "C" void spurious_irq7_handler(void *context) {
#ifdef FKERNEL_DEBUG
  Logf(LogLevel::WARN, "IRQ7 triggered — possibly spurious.");
#endif
  send_eoi(7);
}

extern "C" void rtc_handler(void *context) {
#ifdef FKERNEL_DEBUG
  Logf(LogLevel::TRACE, "Real-Time Clock IRQ8 triggered.");
#endif
  send_eoi(8);
}

extern "C" void acpi_handler(void *context) {
#ifdef FKERNEL_DEBUG
  Logf(LogLevel::TRACE, "ACPI / IRQ2 redirected IRQ9 triggered.");
#endif
  send_eoi(9);
}

extern "C" void irq10_pci_handler(void *context) {
#ifdef FKERNEL_DEBUG
  Logf(LogLevel::TRACE, "IRQ10 triggered (PCI / livre).");
#endif
  send_eoi(10);
}

extern "C" void irq11_pci_handler(void *context) {
#ifdef FKERNEL_DEBUG
  Logf(LogLevel::TRACE, "IRQ11 triggered (PCI / livre).");
#endif
  send_eoi(11);
}

extern "C" void ps2_mouse_handler(void *context) {
#ifdef FKERNEL_DEBUG
  Logf(LogLevel::TRACE, "PS/2 Mouse IRQ12 triggered.");
#endif
  send_eoi(12);
}

extern "C" void fpu_handler(void *context) {
#ifdef FKERNEL_DEBUG
  Logf(LogLevel::TRACE, "FPU / x87 Coprocessor IRQ13 triggered.");
#endif
  send_eoi(13);
}

extern "C" void primary_ata_handler(void *context) {
#ifdef FKERNEL_DEBUG
  Logf(LogLevel::TRACE, "Primary ATA IRQ14 triggered.");
#endif
  send_eoi(14);
}

extern "C" void secondary_ata_handler(void *context) {
#ifdef FKERNEL_DEBUG
  Logf(LogLevel::TRACE, "Secondary ATA IRQ15 triggered.");
#endif
  send_eoi(15);
}
