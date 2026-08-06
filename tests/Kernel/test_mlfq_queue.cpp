// Phase 43e — MLFQ queue unit tests (host-side).
//
// MLFQQueue is a plain struct (IntrusiveList<Task> + quantum/allotment TickCounts).
// Processor::init_quanta() initialises the 4 queues in its constructor.
//
// Tests verify:
//   - Correct number of MLFQ levels (constant)
//   - Processor run_queue quanta match the QoS table (2/4/8/16 ticks)
//   - Intrusive queue push / pop round-trips (FIFO ordering)

#include <tests/test_framework.h>
#include <Kernel/Scheduler/Qos/mlfq_queue.h>
#include <Kernel/Hardware/Cpu/processor.h>
#include <Kernel/Scheduler/Task/task.h>
#include <LibFK/Memory/Allocators/new.h>

using fkernel::scheduler::MLFQ_LEVELS;

static const char* test_mlfq_level_count() {
    TEST_ASSERT_EQ(4, (long)MLFQ_LEVELS, "MLFQ_LEVELS must be 4");
    return nullptr;
}

static const char* test_processor_queue_quanta() {
    fkernel::Processor p;
    TEST_ASSERT_EQ(2,  (long)p.run_queues[0].quantum_ticks.value(), "level 0 quantum must be 2");
    TEST_ASSERT_EQ(4,  (long)p.run_queues[1].quantum_ticks.value(), "level 1 quantum must be 4");
    TEST_ASSERT_EQ(8,  (long)p.run_queues[2].quantum_ticks.value(), "level 2 quantum must be 8");
    TEST_ASSERT_EQ(16, (long)p.run_queues[3].quantum_ticks.value(), "level 3 quantum must be 16");
    return nullptr;
}

static const char* test_processor_queues_start_empty() {
    fkernel::Processor p;
    for (uint32_t i = 0; i < MLFQ_LEVELS; ++i) {
        TEST_ASSERT(p.run_queues[i].queue.empty(),
                    "all MLFQ queues must be empty at construction");
    }
    return nullptr;
}

static const char* test_queue_push_pop_single() {
    fkernel::Processor p;
    Task* t = new Task();

    p.run_queues[0].queue.push_back(t);
    TEST_ASSERT(!p.run_queues[0].queue.empty(), "queue must not be empty after push");
    TEST_ASSERT_EQ(1, (long)p.run_queues[0].queue.size(), "queue size must be 1");

    Task* front = p.run_queues[0].queue.front();
    p.run_queues[0].queue.remove(front);
    TEST_ASSERT(p.run_queues[0].queue.empty(), "queue must be empty after remove");

    delete t;
    return nullptr;
}

static const char* test_queue_push_pop_fifo() {
    fkernel::Processor p;
    Task* t1 = new Task();
    Task* t2 = new Task();
    Task* t3 = new Task();

    p.run_queues[1].queue.push_back(t1);
    p.run_queues[1].queue.push_back(t2);
    p.run_queues[1].queue.push_back(t3);
    TEST_ASSERT_EQ(3, (long)p.run_queues[1].queue.size(), "queue size must be 3");

    Task* first = p.run_queues[1].queue.front();
    TEST_ASSERT(first == t1, "FIFO: first task dequeued must be t1");
    p.run_queues[1].queue.remove(first);

    Task* second = p.run_queues[1].queue.front();
    TEST_ASSERT(second == t2, "FIFO: second task dequeued must be t2");
    p.run_queues[1].queue.remove(second);

    Task* third = p.run_queues[1].queue.front();
    TEST_ASSERT(third == t3, "FIFO: third task dequeued must be t3");
    p.run_queues[1].queue.remove(third);

    TEST_ASSERT(p.run_queues[1].queue.empty(), "queue must be empty after removing all tasks");

    delete t1;
    delete t2;
    delete t3;
    return nullptr;
}

static const char* test_all_queues_empty_helper() {
    fkernel::Processor p;
    TEST_ASSERT(p.all_queues_empty(), "all_queues_empty must return true when all queues are empty");

    Task* t = new Task();
    p.run_queues[2].queue.push_back(t);
    TEST_ASSERT(!p.all_queues_empty(),
                "all_queues_empty must return false when a queue has a task");

    p.run_queues[2].queue.remove(t);
    TEST_ASSERT(p.all_queues_empty(), "all_queues_empty must return true after removing");

    delete t;
    return nullptr;
}

static test_case_t s_tests[] = {
    {"mlfq_level_count",           test_mlfq_level_count},
    {"processor_queue_quanta",     test_processor_queue_quanta},
    {"processor_queues_start_empty", test_processor_queues_start_empty},
    {"queue_push_pop_single",      test_queue_push_pop_single},
    {"queue_push_pop_fifo",        test_queue_push_pop_fifo},
    {"all_queues_empty_helper",    test_all_queues_empty_helper},
};

int run_kernel_mlfq_queue_tests() {
    return run_tests("Kernel::MLFQQueue",
                     s_tests, sizeof(s_tests) / sizeof(s_tests[0]));
}
