#include <LibFK/Algorithms/Logging/log.h>

#include <Kernel/Arch/x86_64/Interrupt/HardwareInterrupts/tick_manager.h>
#include <Kernel/Arch/x86_64/Interrupt/HardwareInterrupts/TimerController/pit.h>
#include <Kernel/Arch/x86_64/Interrupt/HardwareInterrupts/timer_interrupt.h>
#include <Kernel/Arch/x86_64/Interrupt/interrupt_controller.h>
#include <Kernel/Scheduler/Core/scheduler.h>

void TickManager::initialize() {
  m_is_initialized = true;
}

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
  // Bug 26: use PIT channel 2 for accurate early delays (before scheduler).
  PITTimer::pit_wait_ms(static_cast<uint32_t>(ms));
}

void TickManager::increment_ticks() {
  // Bug 32: must be atomic — called from timer ISR on every CPU.
  uint64_t ticks = __sync_add_and_fetch(&m_ticks, 1);
  if (ticks % 1000 == 0)
    fk::algorithms::klog("WATCHDOG", "Heartbeat: System is alive. Total ticks: %llu", ticks);
}
