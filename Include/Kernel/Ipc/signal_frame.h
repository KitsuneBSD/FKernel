#pragma once

#include <Kernel/Arch/x86_64/Syscall/syscall_arch.h>
#include <LibFK/Types/types.h>

namespace fkernel {
namespace ipc {

// Layout pushed on user stack before redirecting to a signal handler.
// Stack at handler entry (rsp → pretcode):
//   [rsp+0]:               pretcode   (restorer address)
//   [rsp+8]:               saved_mask (signal blocked mask to restore on sigreturn)
//   [rsp+16 .. rsp+16+N]:  saved_regs (PtRegs)
//   [after saved_regs]:    builtin_restorer (kernel-provided sigreturn trampoline)
// After handler RET, rsp = rsp+8; restorer calls sys_rt_sigreturn.
struct __attribute__((packed)) KernelSignalFrame {
    uint64_t pretcode;
    uint64_t saved_mask;
    PtRegs   saved_regs;
    // Kernel-provided default restorer: mov eax, __NR_rt_sigreturn; syscall
    // Used when sa_restorer is NULL. pretcode points here in that case.
    uint8_t  builtin_restorer[16];
};

} // namespace ipc
} // namespace fkernel
