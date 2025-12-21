#include <Kernel/Scheduler/Task/task.h>
#include <LibFK/Algorithms/log.h>

Task create_a_new_task(
    TaskId id,
    const fk::text::fixed_string<64>& name,
    void (*entry)(),
    bool is_a_kernel_task,
    bool is_a_idle_task,
    uint8_t priority,
    uint64_t cpu_affinity
) {
    const size_t STACK_SIZE = 16 * fk::types::KiB;
    uint64_t stack_top = reinterpret_cast<uint64_t>(new uint64_t[STACK_SIZE / sizeof(uint64_t)] + (STACK_SIZE / sizeof(uint64_t)));

    Task task{
        .id = id,
        .name = name,

        .state = TaskState::Ready,
        .context = GetContextForNewTask(
            entry,
            stack_top,
            is_a_kernel_task
        ),

        .priority = priority,
        .cpu_affinity = cpu_affinity,

        .is_a_idle_task = is_a_idle_task,
        .is_a_kernel_task = is_a_kernel_task,

        .time_slice_ticks = 0,
        .wake_up_time_ticks = 0,

        .run_node = {},
        .wait_node = {},
        .sleep_node = {},
    };

    return task;
}

void Task::print_info() const {
        fk::algorithms::kdebug("TASK INFO",
                             "Task ID: %lu, Name: %s, State: %u, Priority: %u, CPU Affinity: %lu, Is Idle Task: %s, Is Kernel Task: %s",
                             id,
                             name.c_str(),
                             static_cast<uint8_t>(state),
                             priority,
                             cpu_affinity,
                             is_a_idle_task ? "Yes" : "No",
                             is_a_kernel_task ? "Yes" : "No");
    }