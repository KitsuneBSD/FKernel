#include <Kernel/Fs/Virtual/ProcFs/proc_process_node.h>
#include <Kernel/Scheduler/scheduler.h>
#include <Kernel/Scheduler/Task/task_state.h>
#include <LibFK/Algorithms/log.h>
#include <LibFK/Utilities/memory.h>
#include <LibFK/Text/string.h>

using namespace fk::core;

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
  uint64_t pid = m_pid;
  auto t = SchedulerManager::the().find_task(fk::ProcessId(pid));
  if (!t) {
    const char* msg = "(process not found)\n";
    m_cached.clear();
    for (size_t i = 0; msg[i]; ++i) m_cached.push_back(static_cast<uint8_t>(msg[i]));
    return;
  }

  char tmp[256];
  const char* state = "Unknown";
  {
    fk::synchronization::ScopedLock task_lock(t->lock);
    switch (t->control.lifecycle.state) {
      case TaskState::Running: state = "Running"; break;
      case TaskState::Ready: state = "Ready"; break;
      case TaskState::Blocked: state = "Blocked"; break;
      case TaskState::Sleeping: state = "Sleeping"; break;
      case TaskState::Stopped: state = "Stopped"; break;
      case TaskState::Zombie: state = "Zombie"; break;
    }
    snprintf(tmp, sizeof(tmp), "Name: %s\nPID: %lu\nState: %s\n",
                     t->control.identity.name.c_str(), t->control.identity.id.value(), state);
  }
  fk::text::String buf(tmp);
  m_cached.clear();
  for (size_t i = 0; i < buf.length(); ++i) m_cached.push_back(static_cast<uint8_t>(buf[i]));
}
