#pragma once

#include <Kernel/Arch/x86_64/Syscall/syscall_arch.h>
#include <LibFK/Types/types.h>

namespace fkernel {
namespace ipc {

// Layout pushed on user stack before redirecting to a signal handler.
// Stack at handler entry (rsp → pretcode):
//   [rsp+0]:               pretcode  (restorer address)
//   [rsp+8 .. rsp+8+N]:    saved_regs (PtRegs)
// After handler RET, rsp = rsp+8; sys_rt_sigreturn reads saved_regs from there.
struct __attribute__((packed)) KernelSignalFrame {
    uint64_t pretcode;
    PtRegs   saved_regs;
};

} // namespace ipc
} // namespace fkernel
