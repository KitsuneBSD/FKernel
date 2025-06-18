#include <Arch/x86_64/irq.h>
#include <Arch/x86_64/isr.h>
#include <Driver/Pic.h>
#include <LibFK/Log.h>

IRQHandler irq_handlers[16] = {nullptr};

extern "C" void irq_dispatch(uint8_t irq, void *context) {
  if (irq < 16 && irq_handlers[irq]) {
    irq_handlers[irq](context);
  } else {
    Logf(LogLevel::WARN, "Unhandled IRQ %u", irq);
  }
  send_eoi(irq);
}

void register_irq_handler(uint8_t irq, IRQHandler handler) {
  if (irq < 16) {
    Logf(LogLevel::TRACE, "Install irq %d", irq);
    irq_handlers[irq] = handler;
  } else {
    Logf(LogLevel::ERROR, "Trying to register handler for invalid IRQ %u", irq);
  }
}

extern "C" void (*const irq_stubs[16])() = {
    irq0_handler,  irq1_handler,  irq2_handler,  irq3_handler,
    irq4_handler,  irq5_handler,  irq6_handler,  irq7_handler,
    irq8_handler,  irq9_handler,  irq10_handler, irq11_handler,
    irq12_handler, irq13_handler, irq14_handler, irq15_handler};
