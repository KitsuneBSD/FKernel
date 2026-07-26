#pragma once

#include <Kernel/Arch/x86_64/Syscall/syscall_arch.h>
#include <Kernel/Posix/signal_defs.h>
#include <LibFK/Types/types.h>

namespace fkernel {
namespace ipc {

// Layout pushed on user stack before redirecting to a signal handler.
// Stack at handler entry (rsp -> pretcode):
//   [rsp+0]:               pretcode        (restorer address, 8 bytes)
//   [rsp+8]:               saved_mask      (signal blocked mask, 8 bytes)
//   [rsp+16]:              siginfo         (128 bytes, SA_SIGINFO payload)
//   [rsp+144]:             saved_regs      (PtRegs, 128 bytes)
//   [after saved_regs]:    builtin_restorer (16 bytes)
//
// For !SA_SIGINFO handlers: siginfo is zeroed (handler ignores it via ABI).
// After handler RET, rsp = rsp+8 -> points to saved_mask.
// restorer (pretcode) calls sys_rt_sigreturn.
struct __attribute__((packed)) KernelSignalFrame {
    uint64_t pretcode;
    uint64_t saved_mask;
    siginfo_t siginfo;
    PtRegs    saved_regs;
    uint8_t   builtin_restorer[16];
};

static_assert(sizeof(KernelSignalFrame) == 8 + 8 + 128 + 128 + 16,
              "KernelSignalFrame size mismatch");

}
}
