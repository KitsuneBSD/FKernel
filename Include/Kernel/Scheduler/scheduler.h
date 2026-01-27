#pragma once 

#include <LibFK/Container/intrusive_list.h>
#include <LibFK/Types/types.h>
#include <LibFK/Text/string.h>
#include <LibFK/Algorithms/log.h>

#include <Kernel/Scheduler/Task/task.h>
#include <Kernel/Hardware/Cpu/processor.h>

class SchedulerManager {
private:
    SchedulerManager();

    fkernel::Processor m_processors[32];
    uint32_t m_processor_count = 1;

    fk::containers::IntrusiveList<Task, &Task::wait_node> m_wait_queue;
    fk::containers::IntrusiveList<Task, &Task::sleep_node> m_sleep_queue;
    fk::containers::IntrusiveList<Task, &Task::wait_node> m_zombie_queue;

    bool m_is_initialized = false;
    uint64_t m_default_quantum = 5;
    uint64_t m_next_pid = 1;

public:
    static SchedulerManager& the() {
        static SchedulerManager instance;
        return instance;
    }

    uint64_t generate_pid() { return m_next_pid++; }

    void initialize();
    void add_task(Task* task);
    void block_current();
    void zombify_current();
    void sleep_current(uint64_t ticks);
    void yield();
    void wake_task(Task* task);
    void on_tick();
    void schedule();

    // Debugging / introspection helpers
    void print_all_tasks();
    Task* find_task(TaskId id);
    Task* find_terminated_child(TaskId ppid);
    Task* find_any_child(TaskId ppid);

    Task* pick_next();
    
    fkernel::Processor& current_processor();
    Task* current() { return current_processor().current_task; }
    
    bool is_need_resched() { return current_processor().need_resched; }
    void set_need_resched(bool value) { current_processor().need_resched = value; }
};