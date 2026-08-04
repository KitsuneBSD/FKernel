#include <Kernel/Arch/x86_64/Syscall/syscall_arch.h>
#include <Kernel/Ipc/Capabilities/cspace.h>
#include <Kernel/Ipc/Endpoints/endpoint.h>
#include <Kernel/Ipc/Endpoints/message_info.h>
#include <Kernel/Scheduler/Core/scheduler.h>
#include <Kernel/Syscall/syscall.h>

extern "C" uint64_t sys_ipc_receive(uint64_t handle, uint64_t, uint64_t,
                                    uint64_t, uint64_t, uint64_t, PtRegs* regs) {
  using namespace fkernel::ipc;
  auto *task = SchedulerManager::the().current();
  if (!task || !task->ipc().cspace)
    return -static_cast<uint64_t>(fk::core::Error::PermissionDenied);

  Capability cap = task->ipc().cspace->get(static_cast<uint32_t>(handle));
  if (cap.type() != CapabilityType::Endpoint)
    return (uint64_t)-1;
  if (!cap.can_recv())
    return -static_cast<uint64_t>(fk::core::Error::PermissionDenied);

  Endpoint *endpoint = static_cast<Endpoint *>(cap.object());
  auto result = endpoint->receive();

  if (result.is_error())
    return (uint64_t)-1;

  if (regs) {
    const IpcMessage &message = task->ipc().pending_message;
    regs->rdx = message.word(0);
    regs->r10 = message.word(1);
    regs->r8 = message.word(2);
    regs->r9 = message.word(3);
  }
  return result.value().raw();
}
