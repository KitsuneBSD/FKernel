#pragma once

#include <Kernel/Posix/signal_defs.h>
#include <LibFK/Types/types.h>

struct TaskSignalState {
  uint64_t pending{0};
  uint64_t blocked{0};
  uint64_t forced_pending{0};
  sigaction actions[NSIG];
  uintptr_t trampoline{0};
  int last_info_sig{0};
  siginfo_t last_info{};
};
