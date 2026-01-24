#pragma once

#include <LibFK/Types/types.h>
#include <Kernel/Scheduler/Task/task.h>
#include <LibFK/Container/intrusive_list.h>

namespace fkernel {
namespace ipc {

class Notification {
    uint64_t m_pending_bits{0};
    fk::containers::IntrusiveList<Task, &Task::wait_node> m_waiting_tasks;

public:
    Notification() = default;

    /// @brief Non-blocking signal. Sets the bits and wakes up one waiting task.
    void signal(uint64_t bits);

    /// @brief Blocks until at least one bit is set. Returns the pending bits and clears them.
    uint64_t wait();

    /// @brief Non-blocking poll.
    uint64_t poll();
};

}
}
