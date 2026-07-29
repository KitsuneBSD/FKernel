#include <Kernel/Arch/x86_64/Hardware/Cpu/cpu_ops.h>
#include <Kernel/Arch/x86_64/Interrupt/HardwareInterrupts/tick_manager.h>
#include <Kernel/Arch/x86_64/Interrupt/HardwareInterrupts/timer_interrupt.h>
#include <Kernel/Arch/x86_64/Interrupt/interrupt_controller.h>
#include <Kernel/Scheduler/scheduler.h>
#include <LibFK/Algorithms/log.h>

void TickManager::initialize() {
  if (m_is_initialized) return;
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
  const uint64_t loops_per_ms = 200000;
  uint64_t total = ms * loops_per_ms;
  for (uint64_t i = 0; i < total; ++i) {
    arch_cpu_relax();
  }
}

extern volatile uint64_t g_interrupt_entropy_pool;

void TickManager::increment_ticks() {
    m_ticks++;

    // Accumulate RDTSC jitter into the entropy pool
    uint64_t tsc = arch_read_tsc();
    uint64_t pool = g_interrupt_entropy_pool ^ tsc;
    g_interrupt_entropy_pool = (pool << 17) | (pool >> 47);
    
    // Heartbeat every 1000 ticks
    if (m_ticks % 1000 == 0) {
        fk::algorithms::klog("WATCHDOG", "Heartbeat: System is alive. Total ticks: %llu", m_ticks);
    }
}
