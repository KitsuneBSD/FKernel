#include <Kernel/Arch/x86_64/Syscall/syscall_arch.h>
#include <Kernel/Ipc/signal_delivery.h>
#include <Kernel/Scheduler/scheduler.h>
#include <Kernel/Syscall/syscall.h>
#include <Kernel/Syscall/syscall_utils.h>

// tgkill/tkill: send signal to a specific thread (tid). Since FKernel does not
// implement multi-threading, we treat tgkill/tkill as kill — send to the process.
extern "C" {

uint64_t sys_tgkill(uint64_t tgid, [[maybe_unused]] uint64_t tid, uint64_t sig,
                    uint64_t, uint64_t, uint64_t, [[maybe_unused]] PtRegs* regs) {
  auto task = SchedulerManager::the().find_task(fk::ProcessId((uint64_t)tgid));
  if (!task) return fkernel::return_error(fk::core::Error::NotFound);
  if (sig > 0 && sig < 64)
    fkernel::ipc::SignalDelivery::send_signal(task.get(), (int)sig);
  return 0;
}

uint64_t sys_tkill(uint64_t tid, uint64_t sig,
                   uint64_t, uint64_t, uint64_t, uint64_t, [[maybe_unused]] PtRegs* regs) {
  auto task = SchedulerManager::the().find_task(fk::ProcessId((uint64_t)tid));
  if (!task) return fkernel::return_error(fk::core::Error::NotFound);
  if (sig > 0 && sig < 64)
    fkernel::ipc::SignalDelivery::send_signal(task.get(), (int)sig);
  return 0;
}
}
