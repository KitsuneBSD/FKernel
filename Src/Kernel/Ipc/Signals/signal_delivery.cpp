#include <LibFK/Algorithms/Logging/log.h>
#include <LibFK/Utilities/memory.h>

#include <Kernel/Arch/x86_64/Hardware/Cpu/cpu_ops.h>
#include <Kernel/Fs/Virtual/SignalFd/signal_fd_node.h>
#include <Kernel/Fs/Vfs/Events/kqueue.h>
#include <Kernel/Ipc/Capabilities/cspace.h>
#include <Kernel/Ipc/Notifications/notification.h>
#include <Kernel/Ipc/Signals/signal_delivery.h>
#include <Kernel/Ipc/Signals/signal_frame.h>
#include <Kernel/Ipc/Endpoints/ipc_log_node.h>
#include <Kernel/Hardware/Cpu/cpu_block.h>
#include <Kernel/Memory/UserAccess/user_access.h>
#include <Kernel/Scheduler/Core/scheduler.h>

namespace fkernel {
namespace ipc {

static constexpr uint64_t USERSPACE_MAX = 0x0000800000000000ULL;

static void fill_default_siginfo(siginfo_t& info, int sig) {
  fk::memory::set(&info, 0, sizeof(info));
  info.si_signo = sig;
  info.si_code  = SI_KERNEL;
}

void SignalDelivery::send_signal(Task* target, int signum, const siginfo_t* info,
                                 bool force) {
  if (!target || !target->is_valid() || signum <= 0 || signum >= NSIG) return;
  if (target->control.lifecycle.terminated) return;

  siginfo_t si;
  if (info) {
    fk::memory::copy(&si, info, sizeof(si));
  } else {
    fill_default_siginfo(si, signum);
  }
  si.si_signo = signum;

  fk::synchronization::ScopedLock lock(target->lock);
  target->resources.ipc.signals.pending |= (1ULL << signum);
  if (force)
    target->resources.ipc.signals.forced_pending |= (1ULL << signum);
  target->resources.ipc.signals.last_info_sig = signum;
  target->resources.ipc.signals.last_info = si;

  fk::algorithms::ktrace("SIGNAL", "send_signal(%d): pid=%lu state=%d pending=0x%llx",
                         signum, target->control.identity.id.value(),
                         (int)target->control.lifecycle.state,
                         (unsigned long long)target->resources.ipc.signals.pending);

  if (target->control.lifecycle.state == TaskState::Sleeping ||
      target->control.lifecycle.state == TaskState::Blocked) {
    fk::algorithms::ktrace("SIGNAL", "send_signal(%d): waking blocked/sleeping pid=%lu", signum, target->control.identity.id.value());
    SchedulerManager::the().wake_task(target);
  }

  if (target->resources.ipc.signal_notification)
    target->resources.ipc.signal_notification->signal_with_payload(
        fk::NotificationBits(1ULL << signum), &si, sizeof(si));

  fkernel::notify_signal_kqueue(target, signum);
  fk::algorithms::klog("SIGNAL", "Signal %d sent to PID %lu code=%d",
                       signum, target->control.identity.id.value(), si.si_code);
}

void SignalDelivery::deliver_to_group(int sig, fk::ProcessId tgid, const siginfo_t* info) {
  if (sig <= 0 || sig >= NSIG) return;

  fk::RefPtr<Task> candidate;
  fk::RefPtr<Task> leader;

  uint64_t max_pid = SchedulerManager::the().last_pid().value();
  for (uint64_t probe = 1; probe <= max_pid; ++probe) {
    auto task = SchedulerManager::the().find_task(fk::ProcessId(probe));
    if (!task) continue;
    if (task->control.identity.tgid != tgid) continue;
    if (task->control.lifecycle.terminated) continue;

    if (task->control.identity.id == tgid) leader = task;

    if (!candidate && !(task->resources.ipc.signals.blocked & (1ULL << sig)))
      candidate = task;
  }

  Task* target = candidate ? candidate.get() : (leader ? leader.get() : nullptr);
  if (target) send_signal(target, sig, info);
}

DefaultAction SignalDelivery::classify_default(int sig) {
  switch (sig) {
    case SIGCHLD: case SIGURG: case SIGWINCH: return DefaultAction::Ignore;
    case SIGCONT:                              return DefaultAction::Continue;
    case SIGSTOP: case SIGTSTP:
    case SIGTTIN: case SIGTTOU:               return DefaultAction::Stop;
    default:                                  return DefaultAction::Terminate;
  }
}

void SignalDelivery::apply_default(Task* task, int sig, DefaultAction action) {
  uint64_t pid = task->control.identity.id.value();
  if (action == DefaultAction::Continue) {
    if (task->control.lifecycle.state == TaskState::Stopped) {
      task->control.lifecycle.state = TaskState::Ready;
      SchedulerManager::the().add_task(task);
      fk::algorithms::klog("SIGNAL", "Task %lu continued", pid);
    }
    return;
  }
  if (action == DefaultAction::Stop) {
    if (task->control.lifecycle.state == TaskState::Running ||
        task->control.lifecycle.state == TaskState::Ready) {
      fk::algorithms::klog("SIGNAL", "Stopping Task %lu due to signal %d", pid, sig);
      task->control.lifecycle.state = TaskState::Stopped;
      auto& proc = SchedulerManager::the().current_processor();
      proc.need_resched = true;
    }
    return;
  }
  fk::algorithms::klog("SIGNAL", "Terminating Task %lu due to signal %d", pid, sig);
  if (pid == 1) {
    fk::algorithms::kfatal("SIGNAL", "Init process (PID 1) killed by signal %d", sig);
  }
  task->control.lifecycle.killed_by_signal = true;
  SchedulerManager::the().terminate_current(sig);
}

bool SignalDelivery::install_handler_frame(Task* task, PtRegs* regs, int sig,
                                            const struct sigaction& action,
                                            const siginfo_t& siginfo,
                                            uint64_t orig_syscall) {
  uint64_t pid = task->control.identity.id.value();
  uint64_t handler_addr = reinterpret_cast<uint64_t>(action.sa_handler);

  if (handler_addr >= USERSPACE_MAX) {
    fk::algorithms::kwarn("SIGNAL", "Task %lu: sa_handler %p not in userspace",
                           pid, (void*)handler_addr);
    return false;
  }

  uint64_t restorer = reinterpret_cast<uint64_t>(action.sa_restorer);
  bool use_builtin_restorer = !restorer || restorer >= USERSPACE_MAX;

  bool use_altstack = (action.sa_flags & SA_ONSTACK) != 0
                   && task->resources.ipc.alt_stack.ss_sp != nullptr
                   && !(task->resources.ipc.alt_stack.ss_flags & 2);

  uint64_t user_sp;
  if (use_altstack) {
    user_sp = reinterpret_cast<uint64_t>(task->resources.ipc.alt_stack.ss_sp)
            + task->resources.ipc.alt_stack.ss_size;
  } else {
    user_sp = (regs->rsp & ~15ULL) - 128;
  }

  if (user_sp < sizeof(KernelSignalFrame) + 0x1000ULL || user_sp >= USERSPACE_MAX) {
    fk::algorithms::kwarn("SIGNAL", "Task %lu: stack addr %p invalid for frame push",
                           pid, (void*)user_sp);
    return false;
  }
  user_sp -= sizeof(KernelSignalFrame);

  if (!fkernel::memory::is_user_address(user_sp, sizeof(KernelSignalFrame))) {
    fk::algorithms::kwarn("SIGNAL", "Task %lu: frame addr %p not in user range",
                           pid, (void*)user_sp);
    return false;
  }

  KernelSignalFrame k_frame{};
  if (use_builtin_restorer) {
    k_frame.pretcode = user_sp + offsetof(KernelSignalFrame, builtin_restorer);
    static constexpr uint8_t k_restorer_code[] = {
        0x48, 0xc7, 0xc0, 0x0f, 0x00, 0x00, 0x00,  // mov rax, 15
        0x0f, 0x05,                                   // syscall
        0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90    // nop padding
    };
    fk::memory::copy(k_frame.builtin_restorer, k_restorer_code, sizeof(k_restorer_code));
  } else {
    k_frame.pretcode = restorer;
  }
  k_frame.saved_mask = task->resources.ipc.signals.blocked;
  k_frame.siginfo = siginfo;
  k_frame.saved_regs = *regs;

  static constexpr uint64_t EINTR_RESULT = static_cast<uint64_t>(-4);
  if ((action.sa_flags & SA_RESTART) && k_frame.saved_regs.rax == EINTR_RESULT
          && orig_syscall != 0) {
    k_frame.saved_regs.rip -= 2;
    k_frame.saved_regs.rax  = orig_syscall;
  }

  if (!(action.sa_flags & SA_NODEFER))
    task->resources.ipc.signals.blocked |= (1ULL << sig);
  task->resources.ipc.signals.blocked |= action.sa_mask;
  task->resources.ipc.signals.blocked &= ~((1ULL << SIGKILL) | (1ULL << SIGSTOP));

  auto res = fkernel::memory::copy_to_user(reinterpret_cast<void*>(user_sp),
                                            &k_frame, sizeof(KernelSignalFrame));
  if (res.is_error()) {
    fk::algorithms::kwarn("SIGNAL", "Task %lu: failed to write signal frame", pid);
    return false;
  }

  regs->rsp = user_sp;
  regs->rdi = static_cast<uint64_t>(sig);

  if (action.sa_flags & SA_SIGINFO) {
    regs->rsi = user_sp + offsetof(KernelSignalFrame, siginfo);
    regs->rdx = user_sp + offsetof(KernelSignalFrame, saved_regs);
  }

  regs->rip = handler_addr;

  if (action.sa_flags & SA_RESETHAND)
    task->resources.ipc.signals.actions[sig].sa_handler = SIG_DFL;

  fk::algorithms::klog("SIGNAL", "Task %lu: sig %d frame@%p rsp_orig=%p flags=0x%lx%s%s%s",
                       pid, sig, (void*)user_sp, (void*)k_frame.saved_regs.rsp,
                       (unsigned long)action.sa_flags,
                       (action.sa_flags & SA_SIGINFO)   ? " SA_SIGINFO"   : "",
                       (action.sa_flags & SA_ONSTACK)   ? " SA_ONSTACK"   : "",
                       (action.sa_flags & SA_RESETHAND) ? " SA_RESETHAND" : "");
  fk::algorithms::klog("SIGNAL", "Task %lu redirected to handler %p for signal %d",
                       pid, (void*)handler_addr, sig);
  return true;
}

void SignalDelivery::handle_pending_signals(Task* task, PtRegs* regs,
                                             uint64_t orig_syscall) {
  if (!task || task->control.lifecycle.is_a_kernel_task) return;
  if (!task->has_pending_signals()) return;

  fk::synchronization::ScopedLock lock(task->lock);

  for (int sig = 1; sig < NSIG; ++sig) {
    if (!(task->resources.ipc.signals.pending & (1ULL << sig))) continue;

    // Fatal fault signals (SIGSEGV/SIGILL/...) are force-delivered even if blocked;
    // otherwise the faulting instruction retries forever (livelock).
    bool forced = (task->resources.ipc.signals.forced_pending & (1ULL << sig)) != 0;
    if (!forced && (task->resources.ipc.signals.blocked & (1ULL << sig))) continue;

    task->resources.ipc.signals.pending &= ~(1ULL << sig);
    task->resources.ipc.signals.forced_pending &= ~(1ULL << sig);

    if (task->resources.ipc.signal_fd) {
      if (task->resources.ipc.signal_fd->try_enqueue(sig))
        continue;
    }

    auto& action = task->resources.ipc.signals.actions[sig];

    if (action.sa_handler == SIG_IGN) {
      if (!forced) continue;
      // Force semantics (Linux force_sig): a fatal fault signal runs its default
      // action even against SIG_IGN, or the faulting instruction retries forever.
      action.sa_handler = SIG_DFL;
    }

    if (action.sa_handler == SIG_DFL) {
      DefaultAction da = classify_default(sig);
      if (sig == SIGCHLD) {
        if (da == DefaultAction::Ignore) continue;
        if ((action.sa_flags & SA_NOCLDSTOP) && da == DefaultAction::Stop) continue;
        if (action.sa_flags & SA_NOCLDWAIT) {
          SchedulerManager::the().wake_task(task);
          continue;
        }
      }
      if (da == DefaultAction::Ignore) continue;
      apply_default(task, sig, da);
      return;
    }

    if (!regs) return;

    siginfo_t si{};
    if (task->resources.ipc.signals.last_info_sig == sig) {
      si = task->resources.ipc.signals.last_info;
      task->resources.ipc.signals.last_info_sig = 0;
    } else {
      fill_default_siginfo(si, sig);
    }

    if (install_handler_frame(task, regs, sig, action, si, orig_syscall)) return;
  }
}

}
}
