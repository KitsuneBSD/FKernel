#include <Kernel/Arch/x86_64/Interrupt/HardwareInterrupts/TickManager.h>
#include <LibFK/Algorithms/log.h>

void TickManager::sleep(uint64_t ms) {
  if (m_frequency == 0) {
    fk::algorithms::kwarn("TICK_MANAGER",
                          "Timer frequency not set, cannot sleep accurately.");
    return;
  }
  uint64_t current_ticks = m_ticks;
  uint64_t ticks_to_wait = (ms * m_frequency) / 1000;
  uint64_t end_ticks = current_ticks + ticks_to_wait;

  while (m_ticks < end_ticks) {
    asm volatile("hlt");
  }
}
