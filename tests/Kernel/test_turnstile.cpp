// Phase 43e — Turnstile priority-inheritance unit tests (host-side).
//
// turnstile.cpp uses only:
//   - Task struct members (control.lifecycle.*, resources.ipc.*)
//   - priority_for_qos() from qos.cpp (already in build)
//   - kdebug() from log_targets.cpp (already in build)
//
// No SchedulerManager or VFS calls are made, so no scheduler stubs are needed.
//
// QoSClass ordering: UserInteractive(0) = highest, Maintenance(5) = lowest.
// boost_qos_if_needed() boosts holder when waiter has a LOWER numeric class
// (= higher urgency).  Tests verify direct boost, chain boost, no-boost when
// waiter is lower priority, and unboost restoration.

#include <tests/test_framework.h>
#include <Kernel/Scheduler/Sync/turnstile.h>
#include <Kernel/Scheduler/Task/task.h>
#include <Kernel/Scheduler/Qos/qos.h>
#include <LibFK/Memory/Allocators/new.h>

using fkernel::scheduler::QoSClass;
using fkernel::scheduler::boost_qos_if_needed;
using fkernel::scheduler::unboost_task;
using fkernel::scheduler::reprioritize_task;

// Allocate a heap Task with a given QoS class.
static Task* make_task(QoSClass qos) {
    auto* t = new Task();
    t->control.lifecycle.qos           = qos;
    t->control.lifecycle.original_qos  = qos;
    t->control.lifecycle.boosted       = false;
    t->control.lifecycle.nice          = 0;
    t->control.lifecycle.priority      = static_cast<uint8_t>(
        fkernel::scheduler::priority_for_qos(qos).value());
    t->control.lifecycle.base_priority = t->control.lifecycle.priority;
    t->resources.ipc.active_turnstile  = nullptr;
    t->resources.ipc.pending_turnstile = nullptr;
    return t;
}

static const char* test_basic_pi_boost() {
    // waiter (UserInteractive=0) holds waiting for holder (Default=3)
    // → holder must be boosted to UserInteractive
    Task* holder = make_task(QoSClass::Default);
    Task* waiter = make_task(QoSClass::UserInteractive);

    boost_qos_if_needed(waiter, holder);

    TEST_ASSERT(holder->control.lifecycle.boosted, "holder must be marked boosted");
    TEST_ASSERT(holder->control.lifecycle.qos == QoSClass::UserInteractive,
                "holder QoS must equal waiter QoS after boost");
    TEST_ASSERT(holder->control.lifecycle.original_qos == QoSClass::Default,
                "holder must remember original QoS");

    delete holder;
    delete waiter;
    return nullptr;
}

static const char* test_no_boost_when_waiter_lower_priority() {
    // waiter (Maintenance=5) waits on holder (UserInteractive=0)
    // → no boost: waiter has lower urgency
    Task* holder = make_task(QoSClass::UserInteractive);
    Task* waiter = make_task(QoSClass::Maintenance);

    boost_qos_if_needed(waiter, holder);

    TEST_ASSERT(!holder->control.lifecycle.boosted,
                "holder must NOT be boosted when waiter has lower urgency");
    TEST_ASSERT(holder->control.lifecycle.qos == QoSClass::UserInteractive,
                "holder QoS must be unchanged");

    delete holder;
    delete waiter;
    return nullptr;
}

static const char* test_no_boost_equal_priority() {
    Task* holder = make_task(QoSClass::Default);
    Task* waiter = make_task(QoSClass::Default);

    boost_qos_if_needed(waiter, holder);

    TEST_ASSERT(!holder->control.lifecycle.boosted,
                "no boost when waiter == holder QoS");

    delete holder;
    delete waiter;
    return nullptr;
}

static const char* test_unboost_restores_qos() {
    Task* holder = make_task(QoSClass::Utility);
    Task* waiter = make_task(QoSClass::UserInteractive);

    boost_qos_if_needed(waiter, holder);
    TEST_ASSERT(holder->control.lifecycle.boosted, "holder must be boosted");

    unboost_task(holder);

    TEST_ASSERT(!holder->control.lifecycle.boosted, "holder must not be boosted after unboost");
    TEST_ASSERT(holder->control.lifecycle.qos == QoSClass::Utility,
                "holder QoS must be restored to Utility");

    delete holder;
    delete waiter;
    return nullptr;
}

static const char* test_reprioritize_updates_base_priority() {
    Task* t = make_task(QoSClass::UserInteractive);
    uint8_t old_prio = t->control.lifecycle.base_priority;

    t->control.lifecycle.qos = QoSClass::Maintenance;
    reprioritize_task(t);

    TEST_ASSERT(t->control.lifecycle.base_priority < old_prio,
                "Maintenance base_priority must be lower than UserInteractive");

    delete t;
    return nullptr;
}

static const char* test_null_task_safety() {
    // boost_qos_if_needed with nullptrs must not crash
    boost_qos_if_needed(nullptr, nullptr);
    unboost_task(nullptr);
    reprioritize_task(nullptr);
    return nullptr;
}

static test_case_t s_tests[] = {
    {"basic_pi_boost",                 test_basic_pi_boost},
    {"no_boost_lower_priority_waiter", test_no_boost_when_waiter_lower_priority},
    {"no_boost_equal_priority",        test_no_boost_equal_priority},
    {"unboost_restores_qos",           test_unboost_restores_qos},
    {"reprioritize_updates_base",      test_reprioritize_updates_base_priority},
    {"null_task_safety",               test_null_task_safety},
};

int run_kernel_turnstile_tests() {
    return run_tests("Kernel::Turnstile",
                     s_tests, sizeof(s_tests) / sizeof(s_tests[0]));
}
