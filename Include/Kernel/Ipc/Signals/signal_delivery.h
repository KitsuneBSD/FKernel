#pragma once

#include <Kernel/Ipc/Signals/signal_delivery_default_action.h>
#include <Kernel/Scheduler/Task/task.h>
#include <Kernel/Arch/x86_64/Syscall/syscall_arch.h>
#include <Kernel/Posix/signal_defs.h>

namespace fkernel {
namespace ipc {

class SignalDelivery {
public:
    static void send_signal(Task* target, int signum, const siginfo_t* info = nullptr,
                            bool force = false);
    static void deliver_to_group(int sig, fk::ProcessId tgid, const siginfo_t* info = nullptr);
    static void handle_pending_signals(Task* task, PtRegs* regs = nullptr,
                                       uint64_t orig_syscall = 0);

private:
    static DefaultAction classify_default(int sig);
    static void apply_default(Task* task, int sig, DefaultAction action);
    static bool install_handler_frame(Task* task, PtRegs* regs, int sig,
                                      const struct sigaction& action,
                                      const siginfo_t& siginfo,
                                      uint64_t orig_syscall = 0);
};

}
}
