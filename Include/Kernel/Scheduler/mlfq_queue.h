#pragma once

#include <Kernel/Scheduler/Task/task.h>
#include <LibFK/Container/intrusive_list.h>

namespace fkernel::scheduler {

static constexpr uint32_t MLFQ_LEVELS = 4;

struct MLFQQueue {
    fk::containers::IntrusiveList<Task, &Task::run_node> queue;
    uint8_t quantum_ticks;
    uint64_t allotment_ticks;
};

} // namespace fkernel::scheduler
