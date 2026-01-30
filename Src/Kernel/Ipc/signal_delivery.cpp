#include <Kernel/Ipc/cspace.h>
#include <Kernel/Ipc/notification.h>
#include <Kernel/Ipc/signal_delivery.h>
#include <Kernel/Ipc/ipc_log_node.h>
#include <Kernel/Hardware/Cpu/cpu_block.h>
#include <Kernel/Scheduler/scheduler.h>
#include <LibC/string.h>

extern CpuControlBlock g_cpu_block;

namespace fkernel {
namespace ipc {

void SignalDelivery::handle_pending_signals(Task *task) {
  if (!task || task->control.lifecycle.is_a_kernel_task || !task->resources.ipc.cspace) {
    if (task) {
      uint32_t task_id = task->control.identity.id.value();
      IpcLogNode::the()->log_signal_delivery(task_id, -1, 0); // Invalid task/kernel task
    }
    return;
  }

  uint32_t task_id = task->control.identity.id.value();

  // Signal Notification is at handle 0 by convention
  Capability cap = task->resources.ipc.cspace->get(0);
  if (cap.type() != CapabilityType::Notification) {
    IpcLogNode::the()->log_signal_delivery(task_id, -2, 0); // Not a notification
    return;
  }

  Notification *notification = static_cast<Notification *>(cap.object());
  uint64_t pending = notification->poll();

  if (pending == 0) {
    IpcLogNode::the()->log_signal_delivery(task_id, 0, 0); // No pending signals
    return;
  }

  // Find the first signal bit set
  int sig = 0;
  for (int i = 1; i < 64; ++i) {
    if (pending & (1ULL << i)) {
      sig = i;
      break;
    }
  }

  if (sig == 0 || task->resources.ipc.signals.trampoline == 0) {
    IpcLogNode::the()->log_signal_delivery(task_id, sig, task->resources.ipc.signals.trampoline);
    return;
  }

  IpcLogNode::the()->log_signal_delivery(task_id, sig, task->resources.ipc.signals.trampoline);

  // Save context on user stack and redirect to trampoline
  uint64_t *user_stack = reinterpret_cast<uint64_t *>(task->resources.context.user_rsp);

  // 2. Push current context (Simplified: just RIP and RFLAGS for now)
  *(--user_stack) = task->resources.context.saved_rip;
  *(--user_stack) = task->resources.context.saved_rflags;

  // 3. Update task context to jump to trampoline
  task->resources.context.user_rsp = reinterpret_cast<uint64_t>(user_stack);
  task->resources.context.saved_rip = task->resources.ipc.signals.trampoline;
  
  // Note: We don't have a direct way to set RDI in TaskContext yet, 
  // but we can update the saved PtRegs if it's currently in a syscall.
  // This is a bit tricky since we don't always have a PtRegs pointer here.
  // For now, signals are only delivered on syscall return, so we can 
  // rely on the g_cpu_block update.

  // 4. If this is the current task, we MUST also update g_cpu_block (accessed via gs)
  if (SchedulerManager::the().current() == task) {
      g_cpu_block.user_rsp = task->resources.context.user_rsp;
      g_cpu_block.saved_rip = task->resources.context.saved_rip;
  }
}

} // namespace ipc
} // namespace fkernel