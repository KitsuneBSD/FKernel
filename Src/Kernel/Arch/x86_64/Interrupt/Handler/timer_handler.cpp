#include "Kernel/Arch/x86_64/Interrupt/Handler/interrupt_frame.h"
#include <Kernel/Arch/x86_64/Interrupt/HardwareInterrupts/8259_pic.h>
#include <Kernel/Arch/x86_64/io.h>

uint64_t ticks = 0;

void timer_handler([[maybe_unused]] uint8_t vector, InterruptFrame *frame) {
  (void)frame;
  ticks++;

  PIC8259::send_eoi(0);
}
