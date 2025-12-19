#pragma once 

#include <LibFK/Text/string.h>
#include <LibFK/Container/intrusive_list.h>

#include <Kernel/Scheduler/Task/CpuContext.h>
#include <Kernel/Scheduler/Task/TaskState.h>

// TODO: Change the userID to a proper type based on UUID
using TaskId = uint64_t; 

struct Task {
    TaskId id;
    fk::text::String name;

    TaskState state;
    CpuContext context;

    uint8_t priority; // TODO: Change to enum class for priority levels
    uint64_t cpu_affinity;

    bool is_a_kernel_task;

    uint64_t time_slice_ticks;
    uint64_t wake_up_time_ticks;
    
    fk::containers::IntrusiveListNode<Task> run_node;
    fk::containers::IntrusiveListNode<Task> wait_node;
    fk::containers::IntrusiveListNode<Task> sleep_node;
};

Task create_a_new_task(
    TaskId id,
    const fk::text::String& name,
    bool kernel_task,
    uint8_t priority = 0,
    uint64_t cpu_affinity = ~0ULL
);

