#include <Kernel/Fs/Virtual/ProcFs/proc_pid_exe_node.h>
#include <Kernel/Scheduler/scheduler.h>

fk::core::Result<fk::text::String, fk::core::Error> ProcPidExeNode::read_link() {
  auto t = SchedulerManager::the().find_task(m_pid);
  if (!t) return fk::core::Error::NotFound;
  fk::synchronization::ScopedLock task_lock(t->lock);
  const char* path = t->resources.files.exe_path.c_str();
  if (!path || !path[0]) return fk::text::String("/proc/unknown");
  return fk::text::String(path);
}
