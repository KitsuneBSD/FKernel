#pragma once

#include <Kernel/Scheduler/Task/task.h>
#include <Kernel/Arch/x86_64/Syscall/syscall_arch.h>

namespace fkernel {
namespace ipc {

class SignalDelivery {
public:
    static void send_signal(Task* target, int signum);
    static void handle_pending_signals(Task* task, PtRegs* regs = nullptr,
                                       uint64_t orig_syscall = 0);

private:
    enum class DefaultAction { Ignore, Continue, Stop, Terminate };
    static DefaultAction classify_default(int sig);
    static void apply_default(Task* task, int sig, DefaultAction action);
    static bool install_handler_frame(Task* task, PtRegs* regs, int sig,
                                      const struct sigaction& action,
                                      uint64_t orig_syscall = 0);
};

}
}
