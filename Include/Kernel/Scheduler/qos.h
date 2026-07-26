#pragma once

#include <LibFK/Types/types.h>

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
    uint8_t base_priority;
    uint8_t priority_range;
    uint8_t quantum_ticks;
    uint64_t allotment_ticks;
    uint8_t default_mlfq_level;
};

const QoSLevel& qos_level(QoSClass qos);
uint8_t priority_for_qos(QoSClass qos, int8_t nice = 0);
uint64_t allotment_for_qos(QoSClass qos);
uint8_t quantum_for_level(uint8_t level);
int8_t nice_to_priority_offset(int8_t nice);
QoSClass qos_from_linux_policy(int policy);
int linux_policy_from_qos(SchedulingPolicy policy);

} // namespace fkernel::scheduler
