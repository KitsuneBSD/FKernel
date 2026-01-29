#include <Kernel/Ipc/cspace.h>
#include <Kernel/Ipc/notification.h>
#include <Kernel/Ipc/signal_delivery.h>
#include <Kernel/Hardware/Cpu/cpu_block.h>
#include <Kernel/Scheduler/scheduler.h>
#include <LibC/string.h>

extern CpuControlBlock g_cpu_block;

namespace fkernel {
namespace ipc {

void SignalDelivery::handle_pending_signals(Task *task) {
  if (!task || task->is_a_kernel_task || !task->ipc.cspace)
    return;

  // Signal Notification is at handle 0 by convention
  Capability cap = task->ipc.cspace->get(0);
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

  if (sig == 0 || task->ipc.signals.trampoline == 0)
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
  task->context.rip = task->ipc.signals.trampoline;
  task->context.rdi = sig; // First argument to trampoline

  // 4. If this is the current task, we MUST also update g_cpu_block (accessed via gs)
  // because syscall_stub_post_dispatch restores from there.
  if (SchedulerManager::the().current() == task) {
      g_cpu_block.user_rsp = task->context.rsp;
      g_cpu_block.saved_rip = task->context.rip;
      // We don't strictly need to update saved_rflags here, but it's safer
  }
}

} // namespace ipc
} // namespace fkernel
