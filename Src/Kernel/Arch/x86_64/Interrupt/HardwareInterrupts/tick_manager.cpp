#include <Kernel/Arch/x86_64/Interrupt/HardwareInterrupts/tick_manager.h>

void TickManager::initialize() {
  m_is_initialized = true;
}
#include <Kernel/Arch/x86_64/Interrupt/HardwareInterrupts/timer_interrupt.h>
#include <Kernel/Arch/x86_64/Interrupt/interrupt_controller.h>
#include <Kernel/Scheduler/scheduler.h>
#include <LibFK/Algorithms/log.h>

void TickManager::sleep(uint64_t ms) {
  auto& sched = SchedulerManager::the();
  if (sched.is_initialized() && sched.current()) {
    uint64_t freq = m_frequency > 0 ? m_frequency : 1000;
    uint64_t ticks = (ms * freq) / 1000;
    if (ticks == 0) ticks = 1;
    sched.sleep_current(fk::TickCount(ticks));
    sched.schedule();
    return;
  }
  const uint64_t loops_per_ms = 200000;
  uint64_t total = ms * loops_per_ms;
  for (uint64_t i = 0; i < total; ++i) {
    asm volatile("pause");
  }
}

void TickManager::increment_ticks() {
    m_ticks++;
    
    // Heartbeat every 1000 ticks
    if (m_ticks % 1000 == 0) {
        fk::algorithms::klog("WATCHDOG", "Heartbeat: System is alive. Total ticks: %llu", m_ticks);
    }
}
