#pragma once

#include <Kernel/Scheduler/Qos/qos_class.h>
#include <Kernel/Scheduler/Core/scheduling_policy.h>
#include <Kernel/Scheduler/Qos/qos_level.h>
#include <LibFK/Types/types.h>
#include <LibFK/Types/Process/task_priority.h>
#include <LibFK/Types/Process/nice_value.h>
#include <LibFK/Types/Process/tick_count.h>
#include <LibFK/Types/Process/mlfq_level.h>

namespace fkernel::scheduler {

const QoSLevel& qos_level(QoSClass qos);
fk::TaskPriority priority_for_qos(QoSClass qos, fk::NiceValue nice = fk::NiceValue(0));
fk::TickCount allotment_for_qos(QoSClass qos);
fk::TickCount quantum_for_level(fk::MlqfLevel level);
fk::NiceValue nice_to_priority_offset(fk::NiceValue nice);
QoSClass qos_from_linux_policy(int policy);
int linux_policy_from_qos(SchedulingPolicy policy);

} // namespace fkernel::scheduler
