#include <Kernel/Arch/x86_64/Interrupt/Handler/handlers.h>
#include <Kernel/Arch/x86_64/Interrupt/Handler/interrupt_frame.h>
#include <Kernel/Arch/x86_64/Interrupt/HardwareInterrupts/8259_pic.h>
#include <Kernel/Arch/x86_64/Interrupt/HardwareInterrupts/apic.h>
#include <Kernel/Arch/x86_64/Interrupt/interrupt_controller.h>

#include <Kernel/Hardware/Cpu.h>

extern "C" void interrupt_dispatch(uint8_t vector,
                                   InterruptFrame *frame = nullptr) {
  auto handler = InterruptController::the().get_interrupt(vector);
  if (handler) {
    handler(vector, frame);
  } else
    default_handler(vector, frame);
}
