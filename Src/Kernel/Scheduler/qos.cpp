#include <Kernel/Scheduler/qos.h>

namespace fkernel::scheduler {

static constexpr QoSLevel s_qos_table[6] = {
    {120, 8,  2,   8,   0},  // UserInteractive
    {100, 20, 4,  16,   0},  // UserInitiated
    { 80, 20, 8,  32,   1},  // Default
    { 60, 20, 16, 64,   2},  // Utility
    { 40, 20, 32, 128,  2},  // Background
    { 20, 20, 64, 256,  3},  // Maintenance
};

static constexpr uint8_t s_level_quanta[4] = {2, 4, 8, 16};

const QoSLevel& qos_level(QoSClass qos) {
    return s_qos_table[static_cast<uint8_t>(qos)];
}

uint8_t priority_for_qos(QoSClass qos, int8_t nice) {
    const auto& level = qos_level(qos);
    int16_t offset = nice_to_priority_offset(nice);
    int16_t pri = static_cast<int16_t>(level.base_priority) + offset;
    if (pri < static_cast<int16_t>(level.base_priority) - 8)
        pri = static_cast<int16_t>(level.base_priority) - 8;
    if (pri > static_cast<int16_t>(level.base_priority) + 7)
        pri = static_cast<int16_t>(level.base_priority) + 7;
    return static_cast<uint8_t>(pri);
}

uint64_t allotment_for_qos(QoSClass qos) {
    return qos_level(qos).allotment_ticks;
}

uint8_t quantum_for_level(uint8_t level) {
    if (level >= 4) level = 3;
    return s_level_quanta[level];
}

int8_t nice_to_priority_offset(int8_t nice) {
    if (nice <= -20) return 7;
    if (nice >= 19) return -8;
    return static_cast<int8_t>((nice * -8) / 20);
}

QoSClass qos_from_linux_policy(int policy) {
    switch (policy) {
        case 1: return QoSClass::UserInitiated;   // SCHED_FIFO
        case 2: return QoSClass::UserInitiated;   // SCHED_RR
        case 3: return QoSClass::Utility;          // SCHED_BATCH
        case 5: return QoSClass::Maintenance;      // SCHED_IDLE
        default: return QoSClass::Default;          // SCHED_OTHER
    }
}

int linux_policy_from_qos(SchedulingPolicy policy) {
    switch (policy) {
        case SchedulingPolicy::Fifo:       return 1;
        case SchedulingPolicy::RoundRobin: return 2;
        case SchedulingPolicy::Batch:      return 3;
        case SchedulingPolicy::Idle:       return 5;
        default:                           return 0;
    }
}

} // namespace fkernel::scheduler
