#pragma once

#include <Kernel/Scheduler/Task/task.h>
#include <Kernel/Arch/x86_64/Syscall/syscall_arch.h>

namespace fkernel {
namespace ipc {

class SignalDelivery {
public:
    static void send_signal(Task* target, int signum);
    static void handle_pending_signals(Task* task, PtRegs* regs = nullptr);
};

}
}
