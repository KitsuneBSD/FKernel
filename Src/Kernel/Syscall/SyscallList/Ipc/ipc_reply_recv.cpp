#include <Kernel/Arch/x86_64/Syscall/syscall_arch.h>
#include <Kernel/Ipc/Capabilities/cspace.h>
#include <Kernel/Ipc/Endpoints/endpoint.h>
#include <Kernel/Ipc/Endpoints/message_info.h>
#include <Kernel/Scheduler/Core/scheduler.h>
#include <Kernel/Syscall/syscall.h>

extern "C" uint64_t sys_ipc_reply_recv(uint64_t endpoint_id, uint64_t info_raw,
                                       uint64_t arg1, uint64_t arg2, uint64_t arg3,
                                       uint64_t arg4, PtRegs* regs) {
  using namespace fkernel::ipc;
  auto* task = SchedulerManager::the().current();
  if (!task || !task->ipc().cspace)
    return -static_cast<uint64_t>(fk::core::Error::PermissionDenied);

  Capability cap = task->ipc().cspace->get(static_cast<uint32_t>(endpoint_id));
  if (cap.type() != CapabilityType::Endpoint)
    return (uint64_t)-1;
  if (!cap.can_send() || !cap.can_recv())
    return -static_cast<uint64_t>(fk::core::Error::PermissionDenied);

  Endpoint* endpoint = static_cast<Endpoint*>(cap.object());
  MessageInfo info(info_raw);

  IpcMessage reply;
  reply.set_word(0, arg1);
  reply.set_word(1, arg2);
  reply.set_word(2, arg3);
  reply.set_word(3, arg4);

  auto result = endpoint->reply_recv(info, reply);
  if (result.is_error())
    return (uint64_t)-1;

  if (regs) {
    const IpcMessage& received = task->ipc().pending_message;
    regs->rdx = received.word(0);
    regs->r10 = received.word(1);
    regs->r8  = received.word(2);
    regs->r9  = received.word(3);
  }
  return result.value().raw();
}
