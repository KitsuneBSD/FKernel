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
  Logf(LogLevel::INFO, "Keyboard IRQ - scancode: 0x%02X", scancode);
#endif

  send_eoi(1);
}

extern "C" void cascade_handler(void *context) {
#ifdef FKERNEL_DEBUG
  Logf(LogLevel::INFO, "Cascade IRQ2 triggered.");
#endif

  send_eoi(2);
}

extern "C" void com2_handler(void *context) {
#ifdef FKERNEL_DEBUG
  Logf(LogLevel::INFO, "Serial COM2 IRQ3 triggered.");
#endif
  send_eoi(3);
}

extern "C" void com1_handler(void *context) {
#ifdef FKERNEL_DEBUG
  Logf(LogLevel::INFO, "Serial COM1 IRQ4 triggered.");
#endif
  send_eoi(4);
}

extern "C" void legacy_peripheral_handler(void *context) {
#ifdef FKERNEL_DEBUG
  Logf(LogLevel::INFO, "Legacy peripheral IRQ5 triggered.");
#endif
  send_eoi(5);
}
