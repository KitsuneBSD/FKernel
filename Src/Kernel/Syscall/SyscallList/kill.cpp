#include <Kernel/Arch/x86_64/Syscall/syscall_arch.h>
#include <Kernel/Ipc/global_endpoint_manager.h>
#include <Kernel/Syscall/syscall.h>
#include <LibFK/Algorithms/log.h>
#include <LibFK/Core/Error.h>

extern "C" {

uint64_t sys_kill(uint64_t pid, uint64_t sig, uint64_t, uint64_t, uint64_t,
                  uint64_t, PtRegs* regs) {
  using namespace fkernel::ipc;

  Notification *notification =
      GlobalEndpointManager::the().get_notification(pid);
  if (!notification) {
    return (uint64_t)-static_cast<int>(fk::core::Error::NotFound);
  }

  // Sinais POSIX usam bits. sig 9 (SIGKILL) -> bit (1 << 9)
  notification->signal(1ULL << sig);

  return 0;
}
}
