#include <Kernel/Ipc/cspace.h>
#include <Kernel/Ipc/notification.h>
#include <Kernel/Ipc/signal_delivery.h>
#include <LibC/string.h>

namespace fkernel {
namespace ipc {

void SignalDelivery::handle_pending_signals(Task *task) {
  if (!task || task->is_a_kernel_task || !task->cspace)
    return;

  // Signal Notification is at handle 0 by convention
  Capability cap = task->cspace->get(0);
  if (cap.type() != CapabilityType::Notification)
    return;

  Notification *notification = static_cast<Notification *>(cap.object());
  uint64_t pending = notification->poll();

  if (pending == 0)
    return;

  // Find the first signal bit set
  int sig = 0;
  for (int i = 1; i < 64; ++i) {
    if (pending & (1ULL << i)) {
      sig = i;
      break;
    }
  }

  if (sig == 0 || task->signal_state.trampoline == 0)
    return;

  // Save context on user stack and redirect to trampoline
  // 1. Point to user stack
  uint64_t *user_stack = reinterpret_cast<uint64_t *>(task->context.rsp);

  // 2. Push current context (Simplified: just RIP and RFLAGS for now)
  // In a real kernel, we would push the whole SigContext
  *(--user_stack) = task->context.rip;
  *(--user_stack) = task->context.rflags;

  // 3. Update task context to jump to trampoline
  task->context.rsp = reinterpret_cast<uint64_t>(user_stack);
  task->context.rip = task->signal_state.trampoline;
  task->context.rdi = sig; // First argument to trampoline
}

} // namespace ipc
} // namespace fkernel
