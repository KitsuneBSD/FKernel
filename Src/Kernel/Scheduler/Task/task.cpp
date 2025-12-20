#include <Kernel/Scheduler/Task/task.h>

Task create_a_new_task(
    TaskId id,
    const fk::text::String& name,
    void (*entry)(),
    bool kernel_task,
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
            kernel_task
        ),

        .priority = priority,
        .cpu_affinity = cpu_affinity,

        .is_a_kernel_task = kernel_task,

        .time_slice_ticks = 0,
        .wake_up_time_ticks = 0,

        .run_node = {},
        .wait_node = {},
        .sleep_node = {},
    };

    return task;
}