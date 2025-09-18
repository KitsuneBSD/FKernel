#include <Kernel/Arch/x86_64/Interrupt/idt.h>
#include <Kernel/Arch/x86_64/Interrupt/isr_stubs.h>

static void default_handler([[maybe_unused]] InterruptFrame *frame,
                            [[maybe_unused]] uint8_t vector) {
  kprintf("Unhandled interrupt: vector=%u\n", (unsigned)vector);
  for (;;) {
    asm volatile("hlt");
  }
}

extern "C" void isr_dispatcher(uint8_t vector, InterruptFrame *frame) {
  auto handler = g_idt.get_handler(vector);
  if (handler)
    handler(frame, vector);
  else
    default_handler(frame, vector);
}

void init_idt() {
  g_idt.clear();

  // NOTE: Start loading exceptions between isr0..isr32
  for (uint16_t n = 0; n < 32; ++n) {
    g_idt.set_gate(n, g_isr_stubs[n]);
    g_idt.register_handler(n, default_handler);
  }

  g_idt.load();

  kprintf("IDT Loaded\n");
}
