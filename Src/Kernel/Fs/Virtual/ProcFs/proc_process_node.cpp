#include <Kernel/Fs/Virtual/ProcFs/proc_process_node.h>
#include <Kernel/Scheduler/qos.h>
#include <Kernel/Scheduler/scheduler.h>
#include <Kernel/Scheduler/Task/task_state.h>
#include <LibFK/Algorithms/log.h>
#include <LibFK/Utilities/memory.h>

using namespace fk::core;
using namespace fkernel::scheduler;

static const char* qos_name(QoSClass q) {
  switch (q) {
    case QoSClass::UserInteractive: return "user-interactive";
    case QoSClass::UserInitiated:   return "user-initiated";
    case QoSClass::Default:         return "default";
    case QoSClass::Utility:         return "utility";
    case QoSClass::Background:      return "background";
    case QoSClass::Maintenance:     return "maintenance";
  }
  return "unknown";
}

static const char* policy_name(SchedulingPolicy p) {
  switch (p) {
    case SchedulingPolicy::Normal:     return "normal";
    case SchedulingPolicy::Fifo:       return "fifo";
    case SchedulingPolicy::RoundRobin: return "rr";
    case SchedulingPolicy::Batch:      return "batch";
    case SchedulingPolicy::Idle:       return "idle";
  }
  return "unknown";
}

fk::core::Result<size_t, fk::core::Error> ProcProcessNode::read(uint64_t offset, size_t size, uint8_t* buffer) {
  ensure_cached();
  if (offset >= m_cached.size()) return static_cast<size_t>(0);
  size_t available = m_cached.size() - offset;
  size_t to_copy = (size < available) ? size : available;
  for (size_t i = 0; i < to_copy; ++i) buffer[i] = m_cached[offset + i];
  return to_copy;
}

void ProcProcessNode::ensure_cached() {
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
    fk::synchronization::ScopedLock task_lock(t->lock);
    const char* state = "Unknown";
    switch (t->control.lifecycle.state) {
      case TaskState::Running:  state = "R (running)";  break;
      case TaskState::Ready:    state = "R (runnable)"; break;
      case TaskState::Blocked:  state = "S (sleeping)"; break;
      case TaskState::Sleeping: state = "S (sleeping)"; break;
      case TaskState::Stopped:  state = "T (stopped)";  break;
      case TaskState::Zombie:   state = "Z (zombie)";   break;
    }
    snprintf(buf, sizeof(buf),
             "Name:\t%s\n"
             "State:\t%s\n"
             "Pid:\t%llu\n"
             "PPid:\t%llu\n"
             "Uid:\t%u\t%u\t%u\n"
             "Gid:\t%u\t%u\t%u\n"
             "QoS:\t%s\n"
             "Nice:\t%d\n"
             "Policy:\t%s\n"
             "MLFQ:\t%u\n"
             "Cpus_allowed:\t%llu\n",
             t->control.identity.name.c_str(),
             state,
             (unsigned long long)t->control.identity.id.value(),
             (unsigned long long)t->control.identity.ppid.value(),
             t->control.identity.uid, t->control.identity.euid, t->control.identity.suid,
             t->control.identity.gid, t->control.identity.egid, t->control.identity.sgid,
             qos_name(t->control.lifecycle.qos),
             (int)t->control.lifecycle.nice,
             policy_name(t->control.lifecycle.policy),
             (unsigned)t->control.lifecycle.mlfq_level,
             (unsigned long long)t->control.lifecycle.cpu_affinity);
  }
  m_cached.clear();
  for (size_t i = 0; buf[i]; ++i) m_cached.push_back(static_cast<uint8_t>(buf[i]));
}
