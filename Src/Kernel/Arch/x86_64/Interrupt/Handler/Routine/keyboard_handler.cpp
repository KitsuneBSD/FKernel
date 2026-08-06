#include <LibFK/Algorithms/Logging/log.h>

#include <Kernel/Arch/x86_64/Interrupt/Handler/handlers.h>
#include <Kernel/Arch/x86_64/Interrupt/HardwareInterrupts/hardware_interrupt_manager.h>
#include <Kernel/Driver/Keyboard/ps2_keyboard.h>

void keyboard_handler(uint8_t vector, InterruptFrame *frame) {
  (void)frame;
  PS2Keyboard::the().irq_handler();
  HardwareInterruptManager::the().send_eoi(vector);
}
