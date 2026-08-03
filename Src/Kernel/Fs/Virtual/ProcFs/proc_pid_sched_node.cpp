#include <Kernel/Fs/Virtual/ProcFs/proc_pid_sched_node.h>
#include <Kernel/Scheduler/Qos/qos.h>
#include <Kernel/Scheduler/Core/scheduler.h>
#include <LibFK/Algorithms/Logging/log.h>

using namespace fkernel::scheduler;

fk::core::Result<size_t, fk::core::Error> ProcPidSchedNode::read(uint64_t offset, size_t size, uint8_t* buffer) {
  ensure_cached();
  if (offset >= m_cached.size()) return static_cast<size_t>(0);
  size_t available = m_cached.size() - offset;
  size_t to_copy = (size < available) ? size : available;
  for (size_t i = 0; i < to_copy; ++i) buffer[i] = m_cached[offset + i];
  return to_copy;
}

void ProcPidSchedNode::ensure_cached() {
  if (!m_cached.is_empty()) return;
  auto t = SchedulerManager::the().find_task(m_pid);
  if (!t) {
    const char* msg = "(process not found)\n";
    m_cached.clear();
    for (size_t i = 0; msg[i]; ++i) m_cached.push_back(static_cast<uint8_t>(msg[i]));
    return;
  }

  char buf[512];
  {
    fk::synchronization::ScopedLock lock(t->lock);
    auto& lc = t->control.lifecycle;
    snprintf(buf, sizeof(buf),
             "pid\t\t\t: %llu\n"
             "qos\t\t\t: %u\n"
             "nice\t\t\t: %d\n"
             "base_priority\t\t: %u\n"
             "priority\t\t: %u\n"
             "mlfq_level\t\t: %u\n"
             "policy\t\t\t: %u\n"
             "cpu_affinity\t\t: %llu\n"
             "cpu_time_consumed\t: %llu\n"
             "allotment_ticks\t\t: %llu\n"
             "boosted\t\t\t: %d\n"
             "turnstile_active\t: %d\n",
             (unsigned long long)m_pid.value(),
             (unsigned)static_cast<uint8_t>(lc.qos),
             (int)lc.nice,
             (unsigned)lc.base_priority,
             (unsigned)lc.priority,
             (unsigned)lc.mlfq_level,
             (unsigned)static_cast<uint8_t>(lc.policy),
             (unsigned long long)lc.cpu_affinity,
             (unsigned long long)lc.cpu_time_consumed,
             (unsigned long long)lc.allotment_ticks,
             (int)lc.boosted,
             (int)(t->resources.ipc.active_turnstile != nullptr));
  }
  m_cached.clear();
  for (size_t i = 0; buf[i]; ++i) m_cached.push_back(static_cast<uint8_t>(buf[i]));
}
