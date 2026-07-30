#include <Kernel/Fs/Vfs/node.h>
#include <Kernel/Fs/Vfs/FileLock/file_lock_list.h>

Node::~Node() {
  fk::synchronization::ScopedLockIRQ lock(m_knotes_lock);
  for (auto& knote : m_knotes) {
    knote.kq = nullptr;
  }
  while (!m_knotes.is_empty()) {
    m_knotes.pop_front();
  }
  if (m_lock_list) {
    delete m_lock_list;
    m_lock_list = nullptr;
  }
}

bool Node::try_acquire_lock(fk::ProcessId pid, short l_type, int64_t start, int64_t end,
                             fk::ProcessId *out_conflict_pid, short *out_conflict_type) {
  if (!m_lock_list)
    m_lock_list = new fkernel::FileLockList();
  if (!m_lock_list) return false;
  return m_lock_list->try_acquire(pid, l_type, start, end, out_conflict_pid, out_conflict_type);
}

void Node::release_lock(fk::ProcessId pid, int64_t start, int64_t end) {
  if (m_lock_list)
    m_lock_list->release(pid, start, end);
}

void Node::release_all_locks_for_process(fk::ProcessId pid) {
  if (m_lock_list)
    m_lock_list->release_all_for_process(pid);
}

bool Node::test_lock_conflict(short l_type, int64_t start, int64_t end,
                               fk::ProcessId *out_conflict_pid, short *out_conflict_type) {
  if (!m_lock_list)
    m_lock_list = new fkernel::FileLockList();
  if (!m_lock_list) return false;
  return m_lock_list->test_conflict(l_type, start, end, out_conflict_pid, out_conflict_type);
}
