#pragma once

#include <LibFK/Types/Process/process_id.h>
#include <LibFK/Text/fixed_string.h>
#include <Kernel/Scheduler/Qos/qos.h>

struct Task;

void initialize_task(Task* task, fk::ProcessId id, const fk::text::fixed_string<64>& name,
                     void (*entry)(), bool kernel_task, uint8_t priority,
                     uint64_t cpu_affinity, uint64_t arg1 = 0,
                     uint64_t arg2 = 0,
                     fkernel::scheduler::QoSClass qos = fkernel::scheduler::QoSClass::Default);
