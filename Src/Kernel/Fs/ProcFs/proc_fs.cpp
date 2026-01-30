#include <Kernel/Fs/ProcFs/proc_fs.h>
#include <Kernel/Scheduler/scheduler.h>
#include <Kernel/Driver/Storage/Partitions/partition_manager.h>
#include <LibFK/Algorithms/log.h>
#include <LibFK/Memory/new.h>
#include <LibFK/Text/string.h>

using namespace fk::core;

fk::core::Result<void, fk::core::Error> ProcFsNode::list_dir(fk::containers::Vector<DirectoryEntry>& entries) {
  // Add partitions entry
  DirectoryEntry p_de;
  strncpy(p_de.name, "partitions", sizeof(p_de.name));
  p_de.type = 0; // Regular file
  entries.push_back(p_de);

  // Add an entry for each active PID
  auto& scheduler = SchedulerManager::the();
  
  // This is a bit inefficient because we don't have a clean "all_tasks" list,
  // but we can iterate the queues.
  
  // We'll use a simple loop from 1 up to m_next_pid to find active tasks.
  // In a real system we'd have a task list.
  
  for (uint64_t pid = 1; pid < 1000; ++pid) {
    if (scheduler.find_task(fk::ProcessId(pid))) {
      DirectoryEntry de;
      char pid_str[32];
      snprintf(pid_str, sizeof(pid_str), "%lu", pid);
      strncpy(de.name, pid_str, sizeof(de.name));
      de.type = 1; // It's a directory /proc/PID
      entries.push_back(de);
    }
  }
  
  return {};
}

fk::core::Result<fk::RefPtr<Node>, fk::core::Error> ProcFsNode::lookup(const char* name) {
  // If name is numeric, return a ProcProcessNode for that pid
  if (!name || !*name) return fk::core::Error::NotFound;

  if (strcmp(name, "partitions") == 0) {
    auto res = fk::make_ref<ProcPartitionsNode>().value();
    if (!res) return fk::core::Error::OutOfMemory;
    return fk::RefPtr<Node>(res);
  }

  // Check if numeric
  uint64_t pid = 0;
  const char* p = name;
  while (*p) {
    if (*p < '0' || *p > '9') return fk::core::Error::NotFound;
    pid = pid * 10 + static_cast<uint64_t>(*p - '0');
    ++p;
  }

  auto res = fk::make_ref<ProcProcessNode>(pid).value();
  if (!res) return fk::core::Error::OutOfMemory;
  return fk::RefPtr<Node>(res);
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
  // Build a small status string
  uint64_t pid = m_pid;
  Task* t = SchedulerManager::the().find_task(fk::ProcessId(pid));
  fk::text::String buf;
  if (!t) {
    buf = "(process not found)\n";
  } else {
    char tmp[256];
    const char* state = "Unknown";
    switch (t->control.lifecycle.state) {
      case TaskState::Running: state = "Running"; break;
      case TaskState::Ready: state = "Ready"; break;
      case TaskState::Blocked: state = "Blocked"; break;
      case TaskState::Sleeping: state = "Sleeping"; break;
      case TaskState::Stopped: state = "Stopped"; break;
      case TaskState::Zombie: state = "Zombie"; break;
    }
    int n = snprintf(tmp, sizeof(tmp), "Name: %s\nPID: %lu\nState: %s\n", 
                     t->control.identity.name.c_str(), t->control.identity.id.value(), state);
    if (n > 0) buf = fk::text::String(tmp);
  }

  m_cached.clear();
  for (size_t i = 0; i < buf.length(); ++i) m_cached.push_back(static_cast<uint8_t>(buf[i]));
}

fk::core::Result<size_t, fk::core::Error> ProcPartitionsNode::read(uint64_t offset, size_t size, uint8_t* buffer) {
  ensure_cached();
  if (offset >= m_cached.size()) return static_cast<size_t>(0);
  size_t available = m_cached.size() - offset;
  size_t to_copy = (size < available) ? size : available;
  for (size_t i = 0; i < to_copy; ++i) buffer[i] = m_cached[offset + i];
  return to_copy;
}

void ProcPartitionsNode::ensure_cached() {
  if (!m_cached.is_empty()) return;
  
  fk::text::String buf = "major minor  #blocks  name\n\n";
  auto& partitions = PartitionManager::the().partitions().all();
  
  for (auto& part : partitions) {
    char tmp[256];
    // We don't have major/minor yet, using dummy values
    snprintf(tmp, sizeof(tmp), "   1     %d   %llu %s\n", 
             0, // dummy minor
             part->sector_count().value(),
             part->name().c_str());
    buf = buf + fk::text::String(tmp);
  }

  m_cached.clear();
  for (size_t i = 0; i < buf.length(); ++i) m_cached.push_back(static_cast<uint8_t>(buf[i]));
}
