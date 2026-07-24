#include <Kernel/Arch/x86_64/Interrupt/Handler/interrupt_frame.h>
#include <Kernel/Arch/x86_64/Interrupt/HardwareInterrupts/hardware_interrupt_manager.h>
#include <Kernel/Arch/x86_64/Interrupt/HardwareInterrupts/tick_manager.h>
#include <Kernel/Arch/x86_64/Interrupt/HardwareInterrupts/timer_interrupt.h>
#include <Kernel/Arch/x86_64/io.h>

void clock_handler([[maybe_unused]] uint8_t vector, InterruptFrame *frame) {
  (void)frame;

  outb(0x70, 0x0C);
  (void)inb(0x71);

  HardwareInterruptManager::the().send_eoi(vector);
}
