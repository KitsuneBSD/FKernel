#include <Kernel/Arch/x86_64/Interrupt/Handler/interrupt_frame.h>
#include <Kernel/Arch/x86_64/Interrupt/HardwareInterrupts/HardwareInterrupt.h>
#include <Kernel/Arch/x86_64/Interrupt/HardwareInterrupts/TickManager.h>
#include <Kernel/Arch/x86_64/Interrupt/HardwareInterrupts/TimerInterrupt.h>
#include <Kernel/Arch/x86_64/io.h>
#include <LibFK/Algorithms/log.h>

void clock_handler([[maybe_unused]] uint8_t vector, InterruptFrame *frame) {
  fk::algorithms::kdebug("INTERRUPT ROUTINE", "Triggering Clock Handler");
  (void)frame;

  outb(0x70, 0x0C);
  (void)inb(0x71);

  HardwareInterruptManager::the().send_eoi(vector);
}
