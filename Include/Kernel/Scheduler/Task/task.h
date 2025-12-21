#pragma once 

#include <LibFK/Text/fixed_string.h>
#include <LibFK/Container/intrusive_list.h>

#include <Kernel/Scheduler/Task/cpu_context.h>
#include <Kernel/Scheduler/Task/task_state.h>

// TODO: Change the userID to a proper type based on UUID
using TaskId = uint64_t; 

struct Task {
    TaskId id;
    fk::text::fixed_string<64> name;

    TaskState state;
    CpuContext context;

    uint8_t priority; // TODO: Change to enum class for priority levels
    uint64_t cpu_affinity;

    bool is_a_idle_task {false};
    bool is_a_kernel_task {true};

    uint64_t time_slice_ticks;
    uint64_t wake_up_time_ticks;
    
    fk::containers::IntrusiveListNode<Task> run_node;
    fk::containers::IntrusiveListNode<Task> wait_node;
    fk::containers::IntrusiveListNode<Task> sleep_node;


    void print_info() const;
    bool is_idle() const { return is_a_idle_task; }
};

Task create_a_new_task(
    TaskId id,
    const fk::text::fixed_string<64>& name,
    void (*entry)(),
    bool is_a_kernel_task,
    bool is_a_idle_task,
    uint8_t priority,
    uint64_t cpu_affinity
); 

