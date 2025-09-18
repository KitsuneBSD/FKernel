#include <Kernel/Arch/x86_64/Interrupt/handlerList.h>
#include <Kernel/Arch/x86_64/Interrupt/idt.h>

extern "C" void isr_dispatcher(uint8_t vector, InterruptFrame *frame) {
  auto handler = g_idt.get_isr_handler(vector);
  if (handler)
    handler(frame, vector);
  else
    default_handler(frame, vector);
}

extern "C" void irq_dispatcher(uint8_t vector, InterruptFrame *frame) {
  auto handler = g_idt.get_irq_handler(vector);
  if (handler)
    handler(frame, vector);
}
