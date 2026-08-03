#pragma once

#include <LibFK/Algorithms/Generic/interval.h>
#include <LibFK/Types/Process/process_id.h>
#include <LibFK/Types/types.h>

namespace fkernel {

static constexpr short F_RDLCK = 0;
static constexpr short F_WRLCK = 1;
static constexpr short F_UNLCK = 2;

struct FileLock {
  fk::ProcessId pid;
  short  l_type;   // F_RDLCK=0, F_WRLCK=1
  int64_t l_start; // byte offset
  int64_t l_end;   // inclusive end; INT64_MAX for len==0 "to EOF"

  bool overlaps(int64_t start, int64_t end) const {
    return fk::algorithms::intervals_overlap(l_start, l_end, start, end);
  }
};

} // namespace fkernel
