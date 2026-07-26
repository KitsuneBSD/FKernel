#pragma once

#include <Kernel/Scheduler/Task/task.h>
#include <Kernel/Scheduler/mlfq_queue.h>
#include <LibFK/Container/intrusive_list.h>
#include <LibFK/Synchronization/spinlock.h>

namespace fkernel {

struct Processor {
    uint32_t id;
    Task* current_task { nullptr };
    Task* idle_task { nullptr };
    bool need_resched { false };
    fk::synchronization::Spinlock run_queue_lock;
    scheduler::MLFQQueue run_queues[scheduler::MLFQ_LEVELS];

    Processor() : id(0) {
        init_quanta();
    }
    explicit Processor(uint32_t id) : id(id) {
        init_quanta();
    }

    size_t run_queue_total_size() const {
        size_t total = 0;
        for (uint32_t i = 0; i < scheduler::MLFQ_LEVELS; ++i)
            total += run_queues[i].queue.size();
        return total;
    }

    bool all_queues_empty() const {
        for (uint32_t i = 0; i < scheduler::MLFQ_LEVELS; ++i)
            if (!run_queues[i].queue.empty()) return false;
        return true;
    }

private:
    void init_quanta() {
        run_queues[0].quantum_ticks = 2;
        run_queues[0].allotment_ticks = 8;
        run_queues[1].quantum_ticks = 4;
        run_queues[1].allotment_ticks = 32;
        run_queues[2].quantum_ticks = 16;
        run_queues[2].allotment_ticks = 64;
        run_queues[3].quantum_ticks = 64;
        run_queues[3].allotment_ticks = 256;
    }
};

} // namespace fkernel
