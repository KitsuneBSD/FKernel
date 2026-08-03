#pragma once

#include <Kernel/Scheduler/Task/task_state.h>
#include <Kernel/Scheduler/Qos/qos.h>

struct TaskLifecycle {
  TaskState state;
  uint8_t priority;
  int8_t nice{0};
  uint64_t cpu_affinity;
  uint64_t time_slice_ticks{0};
  uint64_t wake_up_time_ticks{0};
  bool is_a_kernel_task{true};
  bool terminated{false};
  bool killed_by_signal{false};
  int exit_status{0};
  uintptr_t clear_child_tid{0};
  bool vfork_waiting{false};
  fk::ProcessId vfork_parent_id;
  bool is_vfork_sharing_address_space{false};
  bool in_wait_queue{false};
  struct {
    uint64_t remaining_ticks{0};
    uint64_t interval_ticks{0};
    int signo{14};
    bool active{false};
  } itimers[3]{};

  fkernel::scheduler::QoSClass qos{fkernel::scheduler::QoSClass::Default};
  fkernel::scheduler::SchedulingPolicy policy{fkernel::scheduler::SchedulingPolicy::Normal};
  uint8_t base_priority{0};
  uint8_t mlfq_level{0};
  uint64_t cpu_time_consumed{0};
  uint64_t allotment_ticks{0};
  bool boosted{false};
  fkernel::scheduler::QoSClass original_qos{fkernel::scheduler::QoSClass::Default};
};
