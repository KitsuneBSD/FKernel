#pragma once

#include <LibFK/Container/intrusive_list.h>
#include <LibFK/Types/types.h>
#include <LibFK/Text/string.h>
#include <LibFK/Algorithms/log.h>
#include <LibFK/Synchronization/spinlock.h>
#include <LibFK/Memory/ref_ptr.h>

#include <Kernel/Scheduler/Task/task.h>
#include <Kernel/Hardware/Cpu/processor.h>

class SchedulerManager {
private:
    SchedulerManager();

    fk::synchronization::Spinlock m_lock;
    fkernel::Processor m_processors[32];
    uint32_t m_processor_count = 1;

    fk::containers::IntrusiveList<Task, &Task::wait_node> m_wait_queue;
    fk::containers::IntrusiveList<Task, &Task::sleep_node> m_sleep_queue;
    fk::containers::IntrusiveList<Task, &Task::zombie_node> m_zombie_queue;

    bool m_is_initialized = false;
    uint64_t m_default_quantum = 5;
    uint64_t m_next_pid = 1;

public:
    static SchedulerManager& the() {
        static SchedulerManager instance;
        return instance;
    }

    fk::ProcessId generate_pid() {
      return fk::ProcessId(__sync_fetch_and_add(&m_next_pid, 1));
    }

    uint64_t last_pid() const { return m_next_pid; }

    void initialize();
    void add_task(Task* task);
    void block_current();
    void block_current_noqueue();
    void zombify_current();
    void sleep_current(uint64_t ticks);
    void yield();
    void wake_task(Task* task);
    void terminate_current(int status);
    void on_tick();
    void schedule();

    uint64_t process_count();

    // Debugging / introspection helpers
    void print_all_tasks();
    fk::RefPtr<Task> find_task(fk::ProcessId id);
    fk::RefPtr<Task> find_terminated_child(fk::ProcessId ppid);
    fk::RefPtr<Task> find_any_child(fk::ProcessId ppid);
    void reap_zombie(Task* task);
    void send_signal_to_pgrp(int pgid, int signum);

    Task* pick_next();
    Task* steal_task(uint32_t stealing_cpu);

    fkernel::Processor& current_processor();
    Task* current() { return current_processor().current_task; }
    
    bool is_need_resched() { return current_processor().need_resched; }
    void set_need_resched(bool value) { current_processor().need_resched = value; }
    bool is_initialized() const { return m_is_initialized; }
};