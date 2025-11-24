#include <Kernel/Arch/x86_64/Interrupt/Handler/handlers.h>
#include <Kernel/Arch/x86_64/Interrupt/HardwareInterrupts/HardwareInterrupt.h>
#include <Kernel/Driver/Keyboard/ps2_keyboard.h>
#include <LibFK/Algorithms/log.h>

void keyboard_handler(uint8_t vector, InterruptFrame *frame) {
  fk::algorithms::kdebug("INTERRUPT_ROUTINE", "Triggering Keyboard routine");
  (void)frame;
  PS2Keyboard::the().irq_handler();
  HardwareInterruptManager::the().send_eoi(vector);
}
