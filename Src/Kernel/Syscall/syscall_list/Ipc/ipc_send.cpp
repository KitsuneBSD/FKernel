#include <Kernel/Arch/x86_64/Syscall/syscall_arch.h>
#include <Kernel/Ipc/cspace.h>
#include <Kernel/Ipc/endpoint.h>
#include <Kernel/Ipc/message_info.h>
#include <Kernel/Scheduler/scheduler.h>
#include <Kernel/Syscall/syscall.h>

extern "C" uint64_t sys_ipc_send(uint64_t handle, uint64_t info_raw,
                                 [[maybe_unused]] uint64_t arg1,
                                 [[maybe_unused]] uint64_t arg2,
                                 [[maybe_unused]] uint64_t arg3,
                                 [[maybe_unused]] uint64_t arg4, [[maybe_unused]] PtRegs* regs) {
  using namespace fkernel::ipc;
  auto *task = SchedulerManager::the().current();
  if (!task || !task->ipc().cspace)
    return -static_cast<uint64_t>(fk::core::Error::PermissionDenied);

  Capability cap = task->ipc().cspace->get(static_cast<uint32_t>(handle));
  if (cap.type() != CapabilityType::Endpoint)
    return (uint64_t)-1;
  if (!cap.can_send())
    return -static_cast<uint64_t>(fk::core::Error::PermissionDenied);

  Endpoint *endpoint = static_cast<Endpoint *>(cap.object());
  MessageInfo info(info_raw);

  auto result = endpoint->send(info);
  if (result.is_error())
    return (uint64_t)-1;
  return result.value().raw();
}
