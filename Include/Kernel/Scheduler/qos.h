#pragma once

#include <LibFK/Types/types.h>
#include <LibFK/Types/task_priority.h>
#include <LibFK/Types/nice_value.h>
#include <LibFK/Types/tick_count.h>
#include <LibFK/Types/mlfq_level.h>

namespace fkernel::scheduler {

enum class QoSClass : uint8_t {
    UserInteractive  = 0,
    UserInitiated    = 1,
    Default          = 2,
    Utility          = 3,
    Background       = 4,
    Maintenance      = 5,
};

enum class SchedulingPolicy : uint8_t {
    Normal     = 0,
    Fifo       = 1,
    RoundRobin = 2,
    Batch      = 3,
    Idle       = 4,
};

struct QoSLevel {
    fk::TaskPriority base_priority;
    fk::TaskPriority priority_range;
    fk::TickCount quantum_ticks;
    fk::TickCount allotment_ticks;
    fk::MlqfLevel default_mlfq_level;
};

const QoSLevel& qos_level(QoSClass qos);
fk::TaskPriority priority_for_qos(QoSClass qos, fk::NiceValue nice = fk::NiceValue(0));
fk::TickCount allotment_for_qos(QoSClass qos);
fk::TickCount quantum_for_level(fk::MlqfLevel level);
fk::NiceValue nice_to_priority_offset(fk::NiceValue nice);
QoSClass qos_from_linux_policy(int policy);
int linux_policy_from_qos(SchedulingPolicy policy);

} // namespace fkernel::scheduler
