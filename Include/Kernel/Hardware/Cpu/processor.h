#pragma once

#include <Kernel/Scheduler/Task/task.h>
#include <LibFK/Container/intrusive_list.h>
#include <LibFK/Synchronization/spinlock.h>

namespace fkernel {

struct Processor {
    uint32_t id;
    Task* current_task { nullptr };
    Task* idle_task { nullptr };
    bool need_resched { false };
    fk::synchronization::Spinlock run_queue_lock;
    fk::containers::IntrusiveList<Task, &Task::run_node> run_queue;

    Processor() : id(0) {}
    explicit Processor(uint32_t id) : id(id) {}
};

} // namespace fkernel
