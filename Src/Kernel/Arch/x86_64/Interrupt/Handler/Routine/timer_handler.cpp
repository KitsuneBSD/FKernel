#include <LibFK/Algorithms/Logging/log.h>

#include <Kernel/Arch/x86_64/Interrupt/Handler/interrupt_frame.h>
#include <Kernel/Arch/x86_64/Interrupt/HardwareInterrupts/hardware_interrupt_manager.h>
#include <Kernel/Arch/x86_64/Interrupt/HardwareInterrupts/tick_manager.h>
#include <Kernel/Arch/x86_64/Interrupt/HardwareInterrupts/timer_interrupt.h>
#include <Kernel/Arch/x86_64/io.h>
#include <Kernel/Driver/Vga/display.h>
#include <Kernel/Scheduler/Core/scheduler.h>

void timer_handler([[maybe_unused]] uint8_t vector, InterruptFrame *frame) {
  (void)frame;
  TickManager::the().increment_ticks();
  Display::the().background_flush();
  SchedulerManager::the().on_tick();

  HardwareInterruptManager::the().send_eoi(static_cast<uint8_t>(vector - 32));
}
