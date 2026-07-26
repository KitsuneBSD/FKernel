#include <Kernel/Arch/x86_64/Syscall/syscall_arch.h>
#include <Kernel/Ipc/signal_delivery.h>
#include <Kernel/Posix/signal_defs.h>
#include <Kernel/Scheduler/scheduler.h>
#include <Kernel/Syscall/syscall.h>
#include <Kernel/Syscall/syscall_utils.h>

extern "C" {

uint64_t sys_tgkill(uint64_t tgid, [[maybe_unused]] uint64_t tid, uint64_t sig,
                    uint64_t, uint64_t, uint64_t, [[maybe_unused]] PtRegs* regs) {
  auto task = SchedulerManager::the().find_task(fk::ProcessId((uint64_t)tgid));
  if (!task) return fkernel::return_error(fk::core::Error::NotFound);
  if (sig > 0 && sig < 64) {
    siginfo_t info{};
    info.si_signo = (int)sig;
    info.si_code  = SI_TKILL;
    info.si_pid   = SchedulerManager::the().current()->control.identity.id.value();
    info.si_uid   = SchedulerManager::the().current()->control.identity.uid;
    fkernel::ipc::SignalDelivery::send_signal(task.get(), (int)sig, &info);
  }
  return 0;
}

uint64_t sys_tkill(uint64_t tid, uint64_t sig,
                   uint64_t, uint64_t, uint64_t, uint64_t, [[maybe_unused]] PtRegs* regs) {
  auto task = SchedulerManager::the().find_task(fk::ProcessId((uint64_t)tid));
  if (!task) return fkernel::return_error(fk::core::Error::NotFound);
  if (sig > 0 && sig < 64) {
    siginfo_t info{};
    info.si_signo = (int)sig;
    info.si_code  = SI_TKILL;
    info.si_pid   = SchedulerManager::the().current()->control.identity.id.value();
    info.si_uid   = SchedulerManager::the().current()->control.identity.uid;
    fkernel::ipc::SignalDelivery::send_signal(task.get(), (int)sig, &info);
  }
  return 0;
}
}
