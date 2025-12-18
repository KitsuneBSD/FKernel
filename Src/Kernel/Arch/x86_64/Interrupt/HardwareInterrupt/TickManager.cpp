#include <Kernel/Arch/x86_64/Interrupt/HardwareInterrupts/TickManager.h>
#include <Kernel/Arch/x86_64/Interrupt/HardwareInterrupts/TimerInterrupt.h>
#include <Kernel/Arch/x86_64/Interrupt/interrupt_controller.h>
#include <LibFK/Algorithms/log.h>

void TickManager::sleep(uint64_t ms) {
  // Simple busy-wait sleep: do not rely on timer interrupts or tick counter.
  // This uses a CPU pause loop calibrated coarsely per millisecond. It's
  // deliberately simple to avoid deadlocks when hardware timers aren't
  // delivering interrupts (e.g., during APIC calibration).
  const uint64_t loops_per_ms = 200000;
  uint64_t total = ms * loops_per_ms;
  for (uint64_t i = 0; i < total; ++i) {
    asm volatile("pause");
  }
}
