#pragma once

#include <LibFK/Container/Sequence/intrusive_list.h>
#include <LibFK/Container/Associative/hash_map.h>
#include <LibFK/Types/types.h>
#include <LibFK/Types/Process/cpu_count.h>
#include <LibFK/Types/Process/tick_count.h>
#include <LibFK/Types/Process/process_id.h>
#include <LibFK/Text/string.h>
#include <LibFK/Algorithms/Logging/log.h>
#include <LibFK/Synchronization/spinlock.h>
#include <LibFK/Memory/Pointers/ref_ptr.h>

#include <Kernel/Arch/x86_64/arch_defs.h>
#include <Kernel/Scheduler/Task/task.h>
#include <Kernel/Scheduler/Qos/qos.h>
#include <Kernel/Hardware/Cpu/processor.h>

namespace fkernel {

class SchedulerManager {
private:
    SchedulerManager();
    SchedulerManager(const SchedulerManager&) = delete;
    SchedulerManager& operator=(const SchedulerManager&) = delete;

    fk::synchronization::Spinlock m_lock;
    fkernel::Processor m_processors[MAX_CPUS];
    fk::CpuCount m_processor_count{1};

    fk::containers::IntrusiveList<Task, &Task::wait_node> m_wait_queue;
    fk::containers::IntrusiveList<Task, &Task::sleep_node> m_sleep_queue;
    fk::containers::IntrusiveList<Task, &Task::zombie_node> m_zombie_queue;

    // O(1) task lookup registry — maintained alongside the queue lists
    fk::containers::HashMap<fk::ProcessId, Task*> m_task_registry;
    fk::synchronization::Spinlock m_task_registry_lock;

    bool m_is_initialized = false;
    fk::TickCount m_default_quantum{5};
    uint64_t m_next_pid = 1;
    fk::TickCount m_global_tick_counter{0};
    static constexpr uint64_t BOOST_PERIOD_TICKS = 500;

public:
    static SchedulerManager& the() {
        static SchedulerManager instance;
        return instance;
    }

    fk::ProcessId generate_pid() {
      return fk::ProcessId(__sync_fetch_and_add(&m_next_pid, 1));
    }

    fk::ProcessId last_pid() const { return fk::ProcessId(m_next_pid); }

    void initialize();
    void add_task(Task* task);
    void block_current();
    void block_current_noqueue();
    void zombify_current();
    void sleep_current(fk::TickCount ticks);
    void yield();
    void wake_task(Task* task);
    void terminate_current(int status);
    // Safe to call from exception handlers: skips copy_to_user cleanup.
    // Never returns: switches away from the terminated task and halts as a fallback.
    [[noreturn]] void kill_current_from_exception(int signal);
    void on_tick();
    void schedule();
    void switch_to_task(Task* next);
    void requeue_running_task();

    void priority_boost_all();

    uint64_t process_count();

    void print_all_tasks();
    fk::RefPtr<Task> find_task(fk::ProcessId id);
    fk::RefPtr<Task> find_terminated_child(fk::ProcessId ppid);
    fk::RefPtr<Task> find_any_child(fk::ProcessId ppid);
    void reap_zombie(Task* task);
    void send_signal_to_pgrp(int pgid, int signum);

    Task* pick_next();
    Task* steal_task(fk::CpuCount stealing_cpu);

    fkernel::Processor& current_processor();
    Task* current() { return current_processor().current_task; }

    Task* last_fpu_task() { return current_processor().last_fpu_task; }
    void set_last_fpu_task(Task* t) { current_processor().last_fpu_task = t; }

    bool is_need_resched() { return current_processor().need_resched; }
    void set_need_resched(bool value) { current_processor().need_resched = value; }
    bool is_initialized() const { return m_is_initialized; }

    void start_aps();
    void idle_loop();
    fk::CpuCount processor_count() const { return m_processor_count; }
};

} // namespace fkernel
using fkernel::SchedulerManager;
