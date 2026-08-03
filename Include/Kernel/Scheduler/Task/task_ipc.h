#pragma once

#include <LibFK/Types/types.h>
#include <LibFK/Synchronization/spinlock.h>
#include <Kernel/Posix/signal_defs.h>
#include <Kernel/Fs/Vfs/Events/node_knote_list.h>

namespace fkernel::ipc {
  class CSpace;
  class Notification;
}

namespace fkernel {
  class SignalFdNode;
}

namespace fkernel::scheduler {
  struct Turnstile;
}

struct TaskIpc {
  ::fkernel::ipc::CSpace *cspace{nullptr};
  struct {
    uint64_t pending{0};
    uint64_t blocked{0};
    uint64_t forced_pending{0};
    sigaction actions[NSIG];
    uintptr_t trampoline{0};
    int last_info_sig{0};
    siginfo_t last_info{};
  } signals{};
  struct {
    void* ss_sp{nullptr};
    size_t ss_size{0};
    int ss_flags{0};
  } alt_stack{};
  ::fkernel::ipc::Notification *signal_notification{nullptr};
  ::fkernel::SignalFdNode *signal_fd{nullptr};
  ::fkernel::scheduler::Turnstile *pending_turnstile{nullptr};
  uint64_t robust_list_head{0};
  ::fkernel::scheduler::Turnstile *active_turnstile{nullptr};
  ::fkernel::KNoteList proc_knotes;
  mutable fk::synchronization::Spinlock proc_knotes_lock;
  ::fkernel::KNoteList signal_knotes;
  mutable fk::synchronization::Spinlock signal_knotes_lock;
};
