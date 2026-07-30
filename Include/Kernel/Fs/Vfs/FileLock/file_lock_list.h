#pragma once

#include <Kernel/Fs/Vfs/FileLock/file_lock.h>
#include <LibFK/Container/vector.h>
#include <LibFK/Synchronization/spinlock.h>
#include <LibFK/Types/process_id.h>
#include <LibFK/Types/types.h>

namespace fkernel {

class FileLockList {
  mutable fk::synchronization::Spinlock m_lock;
  fk::containers::Vector<FileLock> m_entries;

public:
  FileLockList() = default;

  bool try_acquire(fk::ProcessId pid, short l_type, int64_t start, int64_t end,
                   fk::ProcessId *out_conflict_pid, short *out_conflict_type);

  void release(fk::ProcessId pid, int64_t start, int64_t end);

  void release_all_for_process(fk::ProcessId pid);

  bool test_conflict(short l_type, int64_t start, int64_t end,
                     fk::ProcessId *out_conflict_pid, short *out_conflict_type);
};

} // namespace fkernel
