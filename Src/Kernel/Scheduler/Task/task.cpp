#include <Kernel/Scheduler/Task/Task.h>

Task create_a_new_task(
    TaskId id,
    const fk::text::String& name,
    bool kernel_task,
    uint8_t priority,
    uint64_t cpu_affinity
) {
    Task task{};

    task.id = id;
    task.name = name;

    task.state = TaskState::Ready;

    task.priority = priority;
    task.cpu_affinity = cpu_affinity;
    task.is_a_kernel_task = kernel_task;

    task.time_slice_ticks = 0;
    task.wake_up_time_ticks = 0;

    task.context = {};

    return task;
}