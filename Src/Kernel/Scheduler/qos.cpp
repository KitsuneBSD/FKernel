#include <Kernel/Scheduler/qos.h>

namespace fkernel::scheduler {

static constexpr QoSLevel s_qos_table[6] = {
    {fk::TaskPriority(120), fk::TaskPriority(8),  fk::TickCount(2),   fk::TickCount(8),   fk::MlqfLevel(0)},
    {fk::TaskPriority(100), fk::TaskPriority(20), fk::TickCount(4),   fk::TickCount(16),  fk::MlqfLevel(0)},
    {fk::TaskPriority(80),  fk::TaskPriority(20), fk::TickCount(8),   fk::TickCount(32),  fk::MlqfLevel(1)},
    {fk::TaskPriority(60),  fk::TaskPriority(20), fk::TickCount(16),  fk::TickCount(64),  fk::MlqfLevel(2)},
    {fk::TaskPriority(40),  fk::TaskPriority(20), fk::TickCount(32),  fk::TickCount(128), fk::MlqfLevel(2)},
    {fk::TaskPriority(20),  fk::TaskPriority(20), fk::TickCount(64),  fk::TickCount(256), fk::MlqfLevel(3)},
};

static constexpr uint8_t s_level_quanta[4] = {2, 4, 8, 16};

const QoSLevel& qos_level(QoSClass qos) {
    return s_qos_table[static_cast<uint8_t>(qos)];
}

fk::TaskPriority priority_for_qos(QoSClass qos, fk::NiceValue nice) {
    const auto& level = qos_level(qos);
    int16_t offset = nice_to_priority_offset(nice).value();
    int16_t pri = static_cast<int16_t>(level.base_priority.value()) + offset;
    if (pri < static_cast<int16_t>(level.base_priority.value()) - 8)
        pri = static_cast<int16_t>(level.base_priority.value()) - 8;
    if (pri > static_cast<int16_t>(level.base_priority.value()) + 7)
        pri = static_cast<int16_t>(level.base_priority.value()) + 7;
    return fk::TaskPriority(static_cast<uint8_t>(pri));
}

fk::TickCount allotment_for_qos(QoSClass qos) {
    return qos_level(qos).allotment_ticks;
}

fk::TickCount quantum_for_level(fk::MlqfLevel level) {
    if (level.value() >= 4) level = fk::MlqfLevel(3);
    return fk::TickCount(s_level_quanta[level.value()]);
}

fk::NiceValue nice_to_priority_offset(fk::NiceValue nice) {
    if (nice.value() <= -20) return fk::NiceValue(7);
    if (nice.value() >= 19) return fk::NiceValue(-8);
    return fk::NiceValue(static_cast<int8_t>((nice.value() * -8) / 20));
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
