#include <Kernel/Fs/Virtual/ProcFs/proc_pid_cmdline_node.h>
#include <Kernel/Scheduler/scheduler.h>
#include <LibFK/Algorithms/log.h>

using namespace fk::core;

fk::core::Result<size_t, fk::core::Error> ProcPidCmdlineNode::read(uint64_t offset, size_t size, uint8_t* buffer) {
  auto t = SchedulerManager::the().find_task(m_pid);
  if (!t) return fk::core::Error::NotFound;
  fk::synchronization::ScopedLock task_lock(t->lock);
  const char* name = t->control.identity.name.c_str();
  size_t name_len = 0;
  while (name[name_len]) ++name_len;
  size_t total = name_len + 1;
  if (offset >= total) return static_cast<size_t>(0);
  size_t available = total - (size_t)offset;
  size_t to_copy = (size < available) ? size : available;
  for (size_t i = 0; i < to_copy; ++i) {
    size_t pos = (size_t)offset + i;
    buffer[i] = (pos < name_len) ? static_cast<uint8_t>(name[pos]) : 0;
  }
  return to_copy;
}
