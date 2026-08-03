#include <Kernel/Fs/Virtual/ProcFs/proc_pid_fd_node.h>
#include <Kernel/Fs/Vfs/Core/file_description.h>
#include <Kernel/Scheduler/Core/scheduler.h>
#include <LibFK/Algorithms/Logging/format.h>
#include <LibFK/Utilities/memory.h>

using namespace fk::core;

fk::core::Result<void, fk::core::Error> ProcPidFdDirNode::list_dir(fk::containers::Vector<DirectoryEntry>& entries) {
  auto t = SchedulerManager::the().find_task(m_pid);
  if (!t) return fk::core::Error::NotFound;
  fk::synchronization::ScopedLock task_lock(t->lock);
  auto& descs = t->resources.files.descriptors;
  for (size_t i = 0; i < descs.size(); ++i) {
    if (!descs[i]) continue;
    DirectoryEntry de;
    fk::algorithms::format(de.name, sizeof(de.name), "%zu", i);
    de.type = 0;
    entries.push_back(de);
  }
  return {};
}

fk::core::Result<fk::RefPtr<Node>, fk::core::Error> ProcPidFdDirNode::lookup(const char* name) {
  if (!name || !*name) return fk::core::Error::NotFound;
  int fd = 0;
  for (const char* p = name; *p >= '0' && *p <= '9'; ++p)
    fd = fd * 10 + (*p - '0');
  auto t = SchedulerManager::the().find_task(m_pid);
  if (!t) return fk::core::Error::NotFound;
  {
    fk::synchronization::ScopedLock task_lock(t->lock);
    auto& descs = t->resources.files.descriptors;
    if ((size_t)fd >= descs.size() || !descs[fd])
      return fk::core::Error::NotFound;
  }
  return fk::RefPtr<Node>(fk::make_ref<ProcPidFdEntryNode>(m_pid, fd).value());
}

fk::core::Result<fk::text::String, fk::core::Error> ProcPidFdEntryNode::read_link() {
  auto t = SchedulerManager::the().find_task(m_pid);
  if (!t) return fk::core::Error::NotFound;
  fk::synchronization::ScopedLock task_lock(t->lock);
  auto& descs = t->resources.files.descriptors;
  if ((size_t)m_fd >= descs.size() || !descs[m_fd])
    return fk::core::Error::NotFound;
  auto d = descs[m_fd]->dentry();
  if (!d) return fk::text::String("(unknown)");
  return fk::text::String(d->get_path().c_str());
}
