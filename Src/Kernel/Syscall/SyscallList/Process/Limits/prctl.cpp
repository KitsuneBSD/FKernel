#include <LibFK/Utilities/memory.h>

#include <Kernel/Arch/x86_64/Syscall/syscall_arch.h>
#include <Kernel/Memory/UserAccess/user_access.h>
#include <Kernel/Scheduler/Core/scheduler.h>
#include <Kernel/Syscall/syscall_utils.h>

static constexpr uint64_t PR_SET_NAME       = 15;
static constexpr uint64_t PR_GET_NAME       = 16;
static constexpr uint64_t PR_SET_DUMPABLE   = 4;
static constexpr uint64_t PR_GET_DUMPABLE   = 3;
static constexpr uint64_t PR_SET_PDEATHSIG  = 1;
static constexpr uint64_t PR_GET_PDEATHSIG  = 2;
static constexpr uint64_t PR_SET_NO_NEW_PRIVS = 38;
static constexpr uint64_t PR_GET_NO_NEW_PRIVS = 39;

extern "C" uint64_t sys_prctl(uint64_t option, uint64_t arg2, uint64_t arg3,
                               uint64_t arg4, uint64_t arg5, uint64_t,
                               [[maybe_unused]] PtRegs* regs) {
  (void)arg3; (void)arg4; (void)arg5;

  auto* task = SchedulerManager::the().current();
  if (!task) return (uint64_t)-1;

  if (option == PR_SET_NAME) {
    if (!arg2 || !fkernel::memory::is_user_address(arg2, 1))
      return (uint64_t)-22;
    char name[17];
    if (fkernel::memory::copy_from_user(name, reinterpret_cast<const void*>(arg2), 16).is_error())
      return (uint64_t)-14;
    name[16] = '\0';
    task->control.identity.name = name;
    return 0;
  }

  if (option == PR_GET_NAME) {
    if (!arg2 || !fkernel::memory::is_user_address(arg2, 16))
      return (uint64_t)-22;
    const char* n = task->control.identity.name.c_str();
    size_t len = fk::memory::length(n);
    if (len > 15) len = 15;
    char buf[17];
    fk::memory::copy(buf, n, len);
    buf[len] = '\0';
    if (fkernel::memory::copy_to_user(reinterpret_cast<void*>(arg2), buf, len + 1).is_error())
      return (uint64_t)-14;
    return 0;
  }

  if (option == PR_GET_DUMPABLE)
    return 1; // always dumpable

  if (option == PR_SET_DUMPABLE)
    return 0; // accept silently

  if (option == PR_SET_PDEATHSIG || option == PR_GET_PDEATHSIG)
    return 0;

  if (option == PR_SET_NO_NEW_PRIVS)
    return 0;

  if (option == PR_GET_NO_NEW_PRIVS)
    return 0;

  return (uint64_t)-22; // EINVAL — unknown option
}
