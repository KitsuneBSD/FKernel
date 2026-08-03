#pragma once

#include <LibFK/Types/Process/task_priority.h>
#include <LibFK/Types/Process/tick_count.h>
#include <LibFK/Types/Process/mlfq_level.h>

namespace fkernel::scheduler {

struct QoSLevel {
    fk::TaskPriority base_priority;
    fk::TaskPriority priority_range;
    fk::TickCount quantum_ticks;
    fk::TickCount allotment_ticks;
    fk::MlqfLevel default_mlfq_level;
};

} // namespace fkernel::scheduler
