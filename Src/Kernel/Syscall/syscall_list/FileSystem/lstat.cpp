#include <Kernel/Arch/x86_64/Syscall/syscall_arch.h>
#include <Kernel/Syscall/syscall.h>
#include <Kernel/Syscall/syscall_utils.h>
#include <LibFK/Algorithms/log.h>
#include <LibFK/Core/error.h>

extern "C" uint64_t sys_stat(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, [[maybe_unused]] PtRegs* regs);

extern "C" {

uint64_t sys_lstat(uint64_t path, uint64_t statbuf, uint64_t arg3, uint64_t arg4,
                   uint64_t arg5, uint64_t arg6, [[maybe_unused]] PtRegs* regs) {
  // We don't support symlinks yet, so lstat is same as stat
  return sys_stat(path, statbuf, arg3, arg4, arg5, arg6, regs);
}
}
