#pragma once

#include <Kernel/Scheduler/Task/task.h>
#include <LibFK/Container/Sequence/intrusive_list.h>
#include <LibFK/Types/Process/tick_count.h>

namespace fkernel::scheduler {

static constexpr uint32_t MLFQ_LEVELS = 4;

struct MLFQQueue {
    fk::containers::IntrusiveList<Task, &Task::run_node> queue;
    fk::TickCount quantum_ticks;
    fk::TickCount allotment_ticks;
};

} // namespace fkernel::scheduler
