#include <Kernel/Arch/x86_64/Interrupt/idt.h>
#include <Kernel/Arch/x86_64/Interrupt/isr_stubs.h>
#include <LibC/stdio.h>

static IDT g_idt;

extern "C" void isr_dispatcher(uint8_t vector, InterruptFrame *frame) {
  auto handler = g_idt.get_handler(vector);
  if (handler)
    handler(frame, vector);
  else
    IDT::default_handler(frame, vector);
}

void init_idt() {
  g_idt.clear();

  // NOTE: Start loading exceptions between isr0..isr32
  for (uint16_t n = 0; n < 32; ++n) {
    g_idt.set_gate(n, g_isr_stubs[n]);
    g_idt.register_handler(n, IDT::default_handler);
  }

  g_idt.load();

  kprintf("IDT Loaded\n");
}
