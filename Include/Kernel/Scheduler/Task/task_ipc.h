#pragma once

#include <LibFK/Synchronization/spinlock.h>
#include <LibFK/Types/Ipc/notification_bits.h>
#include <LibFK/Types/types.h>
#include <Kernel/Fs/Vfs/Events/node_knote_list.h>
#include <Kernel/Ipc/Endpoints/fast_ipc_cache.h>
#include <Kernel/Ipc/Endpoints/ipc_message.h>
#include <Kernel/Ipc/Endpoints/message_info.h>
#include <Kernel/Posix/signal_defs.h>
#include <Kernel/Scheduler/Task/task_alt_stack.h>
#include <Kernel/Scheduler/Task/task_signal_state.h>

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
  // Rendezvous slot: deliver_message() writes the incoming message here and
  // the blocked task reads it back after schedule() resumes it.
  ::fkernel::ipc::MessageInfo pending_info{};
  ::fkernel::ipc::IpcMessage pending_message{};
  // Async notification result slot (Notification::signal/wait).
  fk::NotificationBits pending_notification{0};
  // Fastpath cache for the last endpoint capability (Phase 51).
  ::fkernel::ipc::FastIpcCache fast_ipc_cache;
  TaskSignalState signals{};
  TaskAltStack alt_stack{};
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
