#include <Kernel/Arch/x86_64/Syscall/syscall_arch.h>
#include <Kernel/Ipc/cspace.h>
#include <Kernel/Ipc/endpoint.h>
#include <Kernel/Ipc/message_info.h>
#include <Kernel/Scheduler/scheduler.h>
#include <Kernel/Syscall/syscall.h>

extern "C" uint64_t sys_ipc_send(uint64_t handle, uint64_t info_raw, uint64_t arg1,
                                uint64_t arg2, uint64_t arg3, uint64_t arg4, PtRegs* regs);
extern "C" uint64_t sys_ipc_receive(uint64_t handle, uint64_t arg1, uint64_t arg2,
                                   uint64_t arg3, uint64_t arg4, uint64_t arg5, PtRegs* regs);

extern "C" uint64_t sys_ipc_call(uint64_t endpoint_id, uint64_t info_raw,
                                 uint64_t arg1, uint64_t arg2, uint64_t arg3,
                                 uint64_t arg4, PtRegs* regs) {
  // Call is Send + Receive (atomic rendezvous)
  // For now, implemented as sequence
  uint64_t res = sys_ipc_send(endpoint_id, info_raw, arg1, arg2, arg3, arg4);
  if (res == (uint64_t)-1)
    return res;

  return sys_ipc_receive(endpoint_id, 0, 0, 0, 0, 0);
}
