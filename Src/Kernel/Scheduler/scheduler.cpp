#include <Kernel/Arch/x86_64/Interrupt/HardwareInterrupts/tick_manager.h>
#include <Kernel/Scheduler/scheduler.h>
#include <LibFK/Algorithms/log.h>

static Task idle_task;

extern "C" void idle_task_entry() {
    for (;;) {
        asm volatile("hlt");
    }
}

void SchedulerManager::initialize() {
    m_is_initialized = true;

    idle_task = create_a_new_task(
        0,
        "idle",
        idle_task_entry,
        true,
        true,
        0,
        0
    );

    idle_task.state = TaskState::Running;
    m_current = &idle_task;

    fk::algorithms::klog(
        "SCHEDULER MANAGER",
        "Initializing Scheduler Manager (round robin)..."
    );
}

void SchedulerManager::add_task(Task* task) {
    if (!task || task == &idle_task)
        return;

    task->state = TaskState::Ready;
    task->time_slice_ticks = m_default_quantum;
    m_run_queue.push_back(task);
}

Task* SchedulerManager::pick_next() {
    if (!m_run_queue.empty()) {
        Task* next = m_run_queue.front();
        m_run_queue.pop_front();

        next->state = TaskState::Running;
        next->time_slice_ticks = m_default_quantum;
        m_current = next;
        return next;
    }

    m_current = &idle_task;
    return &idle_task;
}

void SchedulerManager::on_tick() {
    uint64_t now = TickManager::the().get_ticks();

    for (auto it = m_sleep_queue.begin(); it != m_sleep_queue.end();) {
        Task* task = &(*it);
        if (task->wake_up_time_ticks <= now) {
            m_sleep_queue.remove(task);
            wake_task(task);
        } else {
            ++it;
        }
    }

    if (!m_current || m_current == &idle_task)
        return;

    if (--m_current->time_slice_ticks == 0) {
        m_current->state = TaskState::Ready;
        m_run_queue.push_back(m_current);
        m_current = nullptr;
        is_need_a_resched = true;
    }
}

void SchedulerManager::sleep_current(uint64_t ticks) {
    if (!m_current || m_current == &idle_task)
        return;

    m_current->state = TaskState::Sleeping;
    m_current->wake_up_time_ticks =
        TickManager::the().get_ticks() + ticks;

    m_sleep_queue.push_back(m_current);
    m_current = nullptr;
    is_need_a_resched = true;
}

void SchedulerManager::wake_task(Task* task) {
    if (!task || task == &idle_task)
        return;

    task->state = TaskState::Ready;
    task->time_slice_ticks = m_default_quantum;
    m_run_queue.push_back(task);
}
