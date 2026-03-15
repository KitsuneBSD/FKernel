#include <Kernel/Arch/x86_64/Syscall/syscall_arch.h>
#include <Kernel/Fs/Vfs/virtual_filesystem.h>
#include <Kernel/Scheduler/scheduler.h>
#include <Kernel/Syscall/syscall_utils.h>
#include <LibC/string.h>

using namespace fkernel;

extern "C" {
uint64_t sys_chown([[maybe_unused]] uint64_t path_ptr, [[maybe_unused]] uint64_t owner,
                   [[maybe_unused]] uint64_t group, uint64_t, uint64_t, uint64_t,
                   [[maybe_unused]] PtRegs* regs) {
  return 0;
}
}
