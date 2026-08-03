#include <Kernel/Arch/x86_64/Syscall/syscall_arch.h>
#include <Kernel/Ipc/Capabilities/capability.h>
#include <Kernel/Ipc/Capabilities/cspace.h>
#include <Kernel/Ipc/Endpoints/endpoint.h>
#include <Kernel/Ipc/Notifications/notification.h>
#include <Kernel/Scheduler/Core/scheduler.h>
#include <Kernel/Syscall/syscall.h>

extern "C" uint64_t sys_cap_grant(uint64_t target_pid, uint64_t handle,
                                  [[maybe_unused]] uint64_t rights_mask,
                                  [[maybe_unused]] uint64_t a3,
                                  [[maybe_unused]] uint64_t a4,
                                  [[maybe_unused]] uint64_t a5,
                                  [[maybe_unused]] PtRegs* regs) {
  using namespace fkernel::ipc;
  auto& scheduler = SchedulerManager::the();
  auto* current = scheduler.current();
  if (!current || !current->ipc().cspace)
    return -static_cast<uint64_t>(fk::core::Error::PermissionDenied);

  auto cap = current->ipc().cspace->get(static_cast<uint32_t>(handle));
  if (!cap.is_valid())
    return static_cast<uint64_t>(-1);
  if (!cap.can_manage())
    return -static_cast<uint64_t>(fk::core::Error::PermissionDenied);

  auto target = scheduler.find_task(fk::ProcessId(static_cast<uint32_t>(target_pid)));
  if (!target || !target->ipc().cspace)
    return -static_cast<uint64_t>(fk::core::Error::NotFound);

  CapabilityRights mask = static_cast<CapabilityRights>(static_cast<uint32_t>(rights_mask));
  uint32_t new_handle = current->ipc().cspace->grant(*target->ipc().cspace,
                                                      static_cast<uint32_t>(handle), mask);
  if (new_handle == INVALID_HANDLE)
    return static_cast<uint64_t>(-1);
  return static_cast<uint64_t>(new_handle);
}
