#pragma once

#include <Kernel/Syscall/syscall_numbers.h>
#include <Kernel/Syscall/syscall_types.h>
#include <LibFK/Types/types.h>

#ifdef __x86_64__
#include <Kernel/Arch/x86_64/Syscall/syscall_arch.h>
#endif

class SyscallManager {
public:
  static SyscallManager &the();

  void initialize();
  void register_syscall(uint64_t num, syscall_function_t fn);
  uint64_t handle(uint64_t num, uint64_t arg1, uint64_t arg2, uint64_t arg3,
                  uint64_t arg4, uint64_t arg5, uint64_t arg6, PtRegs* regs);

private:
  SyscallManager() = default;
  syscall_function_t m_syscall_table[SYS_MAX] = {nullptr};
};

#ifdef __cplusplus
extern "C" {
#endif

uint64_t syscall_dispatcher(uint64_t num, uint64_t arg1, uint64_t arg2,
                            uint64_t arg3, uint64_t arg4, uint64_t arg5,
                            uint64_t arg6, PtRegs* regs);

// Handlers
uint64_t sys_fork(uint64_t arg1, uint64_t arg2, uint64_t arg3, uint64_t arg4,
                  uint64_t arg5, uint64_t arg6, PtRegs* regs);
uint64_t sys_open(uint64_t path, uint64_t flags, uint64_t arg3, uint64_t arg4,
                  uint64_t arg5, uint64_t arg6, PtRegs* regs);
uint64_t sys_close(uint64_t fd, uint64_t arg2, uint64_t arg3, uint64_t arg4,
                   uint64_t arg5, uint64_t arg6, PtRegs* regs);
uint64_t sys_read(uint64_t fd, uint64_t buf, uint64_t count, uint64_t arg4,
                  uint64_t arg5, uint64_t arg6, PtRegs* regs);
uint64_t sys_write(uint64_t fd, uint64_t buf, uint64_t count, uint64_t arg4,
                   uint64_t arg5, uint64_t arg6, PtRegs* regs);
uint64_t sys_lseek(uint64_t fd, uint64_t offset, uint64_t whence, uint64_t arg4,
                   uint64_t arg5, uint64_t arg6, PtRegs* regs);
uint64_t sys_exit(uint64_t status, uint64_t arg2, uint64_t arg3, uint64_t arg4,
                  uint64_t arg5, uint64_t arg6, PtRegs* regs);
uint64_t sys_getuid(uint64_t arg1, uint64_t arg2, uint64_t arg3, uint64_t arg4,
                   uint64_t arg5, uint64_t arg6, PtRegs* regs);
uint64_t sys_geteuid(uint64_t arg1, uint64_t arg2, uint64_t arg3, uint64_t arg4,
                    uint64_t arg5, uint64_t arg6, PtRegs* regs);
uint64_t sys_getgid(uint64_t arg1, uint64_t arg2, uint64_t arg3, uint64_t arg4,
                   uint64_t arg5, uint64_t arg6, PtRegs* regs);
uint64_t sys_getegid(uint64_t arg1, uint64_t arg2, uint64_t arg3, uint64_t arg4,
                    uint64_t arg5, uint64_t arg6, PtRegs* regs);
uint64_t sys_getpgrp(uint64_t arg1, uint64_t arg2, uint64_t arg3, uint64_t arg4,
                    uint64_t arg5, uint64_t arg6, PtRegs* regs);
uint64_t sys_yield(uint64_t arg1, uint64_t arg2, uint64_t arg3, uint64_t arg4,
                   uint64_t arg5, uint64_t arg6, PtRegs* regs);
uint64_t sys_fcntl(uint64_t fd, uint64_t cmd, uint64_t arg, uint64_t arg4,
                   uint64_t arg5, uint64_t arg6, PtRegs* regs);

void initialize_syscalls();

#ifdef __cplusplus
}
#endif // __cplusplus
