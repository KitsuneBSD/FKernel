#include <Kernel/Fs/Vfs/FileLock/file_lock_list.h>
#include <LibFK/Algorithms/log.h>

namespace fkernel {

static bool locks_conflict(short type_a, short type_b) {
  if (type_a == F_WRLCK || type_b == F_WRLCK) return true;
  return false; // F_RDLCK compatible with F_RDLCK
}

bool FileLockList::try_acquire(fk::ProcessId pid, short l_type,
                                int64_t start, int64_t end,
                                fk::ProcessId *out_conflict_pid,
                                short *out_conflict_type) {
  fk::synchronization::ScopedLockIRQ lock(m_lock);

  for (size_t i = 0; i < m_entries.size(); ++i) {
    auto &e = m_entries[i];
    if (e.overlaps(start, end) && locks_conflict(l_type, e.l_type)) {
      if (out_conflict_pid)  *out_conflict_pid  = e.pid;
      if (out_conflict_type) *out_conflict_type = e.l_type;
      return false;
    }
  }

  FileLock entry;
  entry.pid     = pid;
  entry.l_type  = l_type;
  entry.l_start = start;
  entry.l_end   = end;
  m_entries.push_back(entry);
  return true;
}

void FileLockList::release(fk::ProcessId pid, int64_t start, int64_t end) {
  fk::synchronization::ScopedLockIRQ lock(m_lock);

  for (size_t i = 0; i < m_entries.size(); ++i) {
    auto &e = m_entries[i];
    if (e.pid == pid && e.l_start == start && e.l_end == end) {
      m_entries.remove_at(i);
      return;
    }
  }
}

void FileLockList::release_all_for_process(fk::ProcessId pid) {
  fk::synchronization::ScopedLockIRQ lock(m_lock);

  // Swap-and-pop: O(L) total vs O(L²) with remove_at.
  size_t i = 0;
  while (i < m_entries.size()) {
    if (m_entries[i].pid == pid) {
      m_entries[i] = m_entries[m_entries.size() - 1];
      m_entries.pop_back();
    } else {
      ++i;
    }
  }
}

bool FileLockList::test_conflict(short l_type, int64_t start, int64_t end,
                                  fk::ProcessId *out_conflict_pid,
                                  short *out_conflict_type) {
  fk::synchronization::ScopedLockIRQ lock(m_lock);

  for (size_t i = 0; i < m_entries.size(); ++i) {
    auto &e = m_entries[i];
    if (e.overlaps(start, end) && locks_conflict(l_type, e.l_type)) {
      if (out_conflict_pid)  *out_conflict_pid  = e.pid;
      if (out_conflict_type) *out_conflict_type = e.l_type;
      return true;
    }
  }
  return false;
}

} // namespace fkernel
