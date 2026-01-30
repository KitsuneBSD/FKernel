#include <Kernel/Arch/x86_64/Syscall/syscall_arch.h>
#include <Kernel/Ipc/cspace.h>
#include <Kernel/Ipc/endpoint.h>
#include <Kernel/Ipc/message_info.h>
#include <Kernel/Scheduler/scheduler.h>
#include <Kernel/Syscall/syscall.h>

extern "C" uint64_t sys_ipc_receive(uint64_t handle, uint64_t, uint64_t,
                                    uint64_t, uint64_t, uint64_t, [[maybe_unused]] PtRegs* regs) {
  using namespace fkernel::ipc;
  auto *task = SchedulerManager::the().current();
  if (!task || !task->ipc().cspace)
    return -static_cast<uint64_t>(fk::core::Error::PermissionDenied);

  Capability cap = task->ipc().cspace->get(static_cast<uint32_t>(handle));
  if (cap.type() != CapabilityType::Endpoint)
    return (uint64_t)-1;

  Endpoint *endpoint = static_cast<Endpoint *>(cap.object());
  auto result = endpoint->receive();

  if (result.is_error())
    return (uint64_t)-1;
  return result.value().raw();
}
