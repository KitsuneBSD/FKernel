#pragma once

#include <Kernel/Scheduler/Task/task.h>

namespace fkernel {
namespace ipc {

class SignalDelivery {
public:
    static void handle_pending_signals(Task* task);
};

}
}
