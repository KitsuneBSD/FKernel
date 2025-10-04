#include <Kernel/Arch/x86_64/Interrupt/Handler/interrupt_frame.h>
#include <Kernel/Arch/x86_64/Interrupt/HardwareInterrupts/apic.h>
#include <Kernel/Arch/x86_64/io.h>

uint64_t apic_ticks = 0;

void apic_timer_handler([[maybe_unused]] uint8_t vector, InterruptFrame *frame) {
  (void)frame;
  apic_ticks++;

  APIC::the().send_eoi();
}
