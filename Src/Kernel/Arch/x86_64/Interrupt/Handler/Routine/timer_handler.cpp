#include <Kernel/Arch/x86_64/Interrupt/Handler/interrupt_frame.h>
#include <Kernel/Arch/x86_64/Interrupt/HardwareInterrupts/hardware_interrupt.h>
#include <Kernel/Arch/x86_64/Interrupt/HardwareInterrupts/tick_manager.h>
#include <Kernel/Arch/x86_64/Interrupt/HardwareInterrupts/timer_interrupt.h>
#include <Kernel/Arch/x86_64/io.h>
#include <LibFK/Algorithms/log.h>

void timer_handler([[maybe_unused]] uint8_t vector, InterruptFrame *frame) {
  (void)frame;
  TickManager::the().increment_ticks();

  HardwareInterruptManager::the().send_eoi(vector);
}
