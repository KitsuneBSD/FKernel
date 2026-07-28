#include <Kernel/Fs/Virtual/ProcFs/proc_loadavg_node.h>
#include <Kernel/Scheduler/scheduler.h>
#include <Kernel/Arch/x86_64/Interrupt/HardwareInterrupts/tick_manager.h>
#include <Kernel/Scheduler/Task/task_state.h>
#include <LibFK/Algorithms/log.h>
#include <LibFK/Utilities/memory.h>

using namespace fk::core;

void ProcLoadavgNode::ensure_cached() {
  if (!m_cached.is_empty()) return;
  auto& scheduler = SchedulerManager::the();
  uint64_t running = 0;
  uint64_t total = scheduler.process_count();
  uint64_t max_pid = scheduler.last_pid().value();
  for (uint64_t pid = 1; pid < max_pid; ++pid) {
    auto t = scheduler.find_task(fk::ProcessId(pid));
    if (t) {
      fk::synchronization::ScopedLock task_lock(t->lock);
      if (t->control.lifecycle.state == TaskState::Running ||
          t->control.lifecycle.state == TaskState::Ready)
        ++running;
    }
  }
  uint64_t ticks = TickManager::the().get_ticks();
  uint32_t freq = TickManager::the().get_frequency();
  uint64_t uptime = (freq > 0) ? (ticks / freq) : 0;
  char buf[128];
  int len = snprintf(buf, sizeof(buf), "%llu.%02llu %llu.%02llu %llu/%llu %llu\n",
                     (unsigned long long)(uptime / 3600),
                     (unsigned long long)((uptime % 3600) * 100 / 3600),
                     (unsigned long long)(uptime / 60),
                     (unsigned long long)((uptime % 60) * 100 / 60),
                     (unsigned long long)running,
                     (unsigned long long)total,
                     (unsigned long long)total);
  m_cached.clear();
  for (int i = 0; i < len; ++i) m_cached.push_back(static_cast<uint8_t>(buf[i]));
}

fk::core::Result<size_t, fk::core::Error> ProcLoadavgNode::read(uint64_t offset, size_t size, uint8_t* buffer) {
  ensure_cached();
  if (offset >= m_cached.size()) return static_cast<size_t>(0);
  size_t available = m_cached.size() - (size_t)offset;
  size_t to_copy = (size < available) ? size : available;
  for (size_t i = 0; i < to_copy; ++i) buffer[i] = m_cached[(size_t)offset + i];
  return to_copy;
}
