#pragma once 

#include <LibFK/Container/intrusive_list.h>
#include <LibFK/Types/types.h>
#include <LibFK/Text/string.h>
#include <LibFK/Algorithms/log.h>

#include <Kernel/Scheduler/Task/task.h>

class SchedulerManager {
private:
    SchedulerManager() = default;

    Task* m_current = nullptr;
    fk::containers::IntrusiveList<Task, &Task::run_node> m_run_queue;
    fk::containers::IntrusiveList<Task, &Task::wait_node> m_wait_queue;
    fk::containers::IntrusiveList<Task, &Task::sleep_node> m_sleep_queue;

    bool m_is_initialized = false;
    bool is_need_a_resched = false;
    uint64_t m_default_quantum = 5;
public:
    static SchedulerManager& the() {
        static SchedulerManager instance;
        return instance;
    }

    void initialize();
    void add_task(Task* task);
    void block_current();
    void sleep_current(uint64_t ticks);
    void wake_task(Task* task);
    void on_tick();

    Task* pick_next();
    Task* current(){return m_current ? m_current : nullptr;}
};