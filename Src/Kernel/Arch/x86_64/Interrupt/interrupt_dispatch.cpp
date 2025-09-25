#include <Kernel/Arch/x86_64/Interrupt/Handler/handlers.h>
#include <Kernel/Arch/x86_64/Interrupt/interrupt_controller.h>

extern "C" void interrupt_dispatch(uint8_t vector) {
  auto handler = InterruptController::the().get_interrupt(vector);
  if (handler)
    handler(vector);
  else
    default_handler(vector);
}
