#include <LibFK/Algorithms/Logging/log.h>
#include <LibFK/Utilities/memory.h>

#include <Kernel/Fs/Virtual/ProcFs/proc_pid_stat_node.h>
#include <Kernel/Scheduler/Qos/qos.h>
#include <Kernel/Scheduler/Core/scheduler.h>
#include <Kernel/Scheduler/Task/task_state.h>

using namespace fk::core;

static char pid_state_char(TaskState s) {
  switch (s) {
    case TaskState::Running:  return 'R';
    case TaskState::Ready:    return 'R';
    case TaskState::Blocked:  return 'S';
    case TaskState::Sleeping: return 'S';
    case TaskState::Stopped:  return 'T';
    case TaskState::Zombie:   return 'Z';
  }
  return 'S';
}

void ProcPidStatNode::ensure_cached() {
  if (!m_cached.is_empty()) return;
  auto t = SchedulerManager::the().find_task(m_pid);
  char buf[384];
  if (!t) {
    snprintf(buf, sizeof(buf), "%llu (unknown) Z 0 0 0 0 -1 0 0 0 0 0 0 0 0 0 0 1 0 0 0 0 0\n",
             (unsigned long long)m_pid.value());
    m_cached.clear();
    for (size_t i = 0; buf[i]; ++i) TRY_OR_FATAL(m_cached.push_back(static_cast<uint8_t>(buf[i])));
    return;
  }
  {
    fk::synchronization::ScopedLock task_lock(t->lock);
    char state = pid_state_char(t->control.lifecycle.state);
    snprintf(buf, sizeof(buf),
             "%llu (%s) %c %llu %llu %llu 0 -1 0 0 0 0 0 0 0 0 0 0 1 0 0 0 0 0 "
             "%llu %llu %llu %llu %llu\n",
             (unsigned long long)m_pid.value(),
             t->control.identity.name.c_str(),
             state,
             (unsigned long long)t->control.identity.ppid.value(),
             (unsigned long long)t->control.identity.pgid.value(),
             (unsigned long long)t->control.identity.sid.value(),
             (unsigned long long)static_cast<uint8_t>(t->control.lifecycle.qos),
             (unsigned long long)t->control.lifecycle.nice,
             (unsigned long long)static_cast<uint8_t>(t->control.lifecycle.policy),
             (unsigned long long)t->control.lifecycle.mlfq_level,
             (unsigned long long)t->control.lifecycle.cpu_affinity);
  }
  m_cached.clear();
  for (size_t i = 0; buf[i]; ++i) TRY_OR_FATAL(m_cached.push_back(static_cast<uint8_t>(buf[i])));
}

fk::core::Result<size_t, fk::core::Error> ProcPidStatNode::read(uint64_t offset, size_t size, uint8_t* buffer) {
  ensure_cached();
  if (offset >= m_cached.size()) return static_cast<size_t>(0);
  size_t available = m_cached.size() - (size_t)offset;
  size_t to_copy = (size < available) ? size : available;
  for (size_t i = 0; i < to_copy; ++i) buffer[i] = m_cached[(size_t)offset + i];
  return to_copy;
}
