#include <Kernel/Ipc/cspace.h>
#include <Kernel/Ipc/notification.h>
#include <Kernel/Ipc/signal_delivery.h>
#include <Kernel/Ipc/signal_frame.h>
#include <Kernel/Ipc/ipc_log_node.h>
#include <Kernel/Hardware/Cpu/cpu_block.h>
#include <Kernel/Scheduler/scheduler.h>
#include <LibC/string.h>
#include <LibFK/Algorithms/log.h>

extern CpuControlBlock g_cpu_block;

namespace fkernel {
namespace ipc {

void SignalDelivery::send_signal(Task* target, int signum) {
    if (!target || signum <= 0 || signum >= NSIG) return;

    fk::synchronization::ScopedLock lock(target->lock);
    
    // 1. Mark pending bit
    target->resources.ipc.signals.pending |= (1 << signum);

    // 2. Wake up if sleeping (interruptible)
    if (target->control.lifecycle.state == TaskState::Sleeping || 
        target->control.lifecycle.state == TaskState::Blocked) {
        SchedulerManager::the().wake_task(target);
    }

    // 3. Notify via IPC (integrates with kqueue/select)
    if (target->resources.ipc.signal_notification) {
        target->resources.ipc.signal_notification->signal(1ULL << signum);
    }
    
    fk::algorithms::klog("SIGNAL", "Signal %d sent to PID %lu", signum, target->control.identity.id.value());
}

void SignalDelivery::handle_pending_signals(Task *task, PtRegs* regs) {
  if (!task || task->control.lifecycle.is_a_kernel_task) return;
  if (!task->has_pending_signals()) return;

  fk::synchronization::ScopedLock lock(task->lock);

  for (int sig = 1; sig < NSIG; ++sig) {
      if ((task->resources.ipc.signals.pending & (1 << sig)) && 
          !(task->resources.ipc.signals.blocked & (1 << sig))) {
          
          // Clear pending bit
          task->resources.ipc.signals.pending &= ~(1 << sig);

          auto& action = task->resources.ipc.signals.actions[sig];

          if (action.sa_handler == SIG_IGN) continue;

          if (action.sa_handler == SIG_DFL) {
              switch (sig) {
                  case SIGCHLD:
                  case SIGURG:
                  case SIGWINCH:
                      continue; // Ignore
                  case SIGKILL:
                  case SIGSEGV:
                  case SIGILL:
                  case SIGFPE:
                  case SIGINT:
                  case SIGTERM:
                  default:
                      fk::algorithms::klog("SIGNAL", "Terminating Task %lu due to signal %d", 
                          task->control.identity.id.value(), sig);
                      SchedulerManager::the().terminate_current(128 + sig);
                      return;
              }
          }

          // Custom Handler
          if (!regs)
              return;

          uint64_t handler_addr = reinterpret_cast<uint64_t>(action.sa_handler);

          // Reject kernel-space handler addresses
          static constexpr uint64_t USERSPACE_MAX = 0x0000800000000000ULL;
          if (handler_addr >= USERSPACE_MAX) {
              fk::algorithms::kerror("SIGNAL", "Task %lu: sa_handler %p is not in userspace — ignoring",
                  task->control.identity.id.value(), (void*)handler_addr);
              continue;
          }

          // Build full signal frame on user stack.
          // Frame layout (rsp → pretcode, then saved_regs):
          //   When handler does RET, rsp advances by 8 (past pretcode) and
          //   we are at the restorer which calls sys_rt_sigreturn.
          //   sys_rt_sigreturn reads saved_regs from [regs->rsp].
          uint64_t restorer = reinterpret_cast<uint64_t>(action.sa_restorer);
          if (!restorer || restorer >= USERSPACE_MAX) restorer = 0;

          uint64_t user_sp = regs->rsp & ~15ULL;
          user_sp -= sizeof(KernelSignalFrame);

          auto* frame = reinterpret_cast<KernelSignalFrame*>(user_sp);
          frame->pretcode  = restorer;
          frame->saved_regs = *regs;

          regs->rsp = user_sp;
          regs->rdi = static_cast<uint64_t>(sig);
          regs->rip = handler_addr;

          fk::algorithms::klog("SIGNAL", "Redirecting Task %lu to handler %p for signal %d",
              task->control.identity.id.value(), (void*)regs->rip, sig);

          return;
      }
  }
}

} // namespace ipc
} // namespace fkernel