#include <tests/test_framework.h>
#include <Kernel/Scheduler/Qos/qos.h>
#include <Kernel/Scheduler/Core/scheduling_policy.h>

using namespace fkernel::scheduler;

static const char* test_qos_level_user_interactive() {
    const QoSLevel& lvl = qos_level(QoSClass::UserInteractive);
    TEST_ASSERT_EQ(120, (long)lvl.base_priority.value(), "UserInteractive base_priority must be 120");
    TEST_ASSERT_EQ(2,   (long)lvl.quantum_ticks.value(), "UserInteractive quantum must be 2 ticks");
    TEST_ASSERT_EQ(8,   (long)lvl.allotment_ticks.value(), "UserInteractive allotment must be 8 ticks");
    TEST_ASSERT_EQ(0,   (long)lvl.default_mlfq_level.value(), "UserInteractive must map to MLFQ level 0");
    return nullptr;
}

static const char* test_qos_level_maintenance() {
    const QoSLevel& lvl = qos_level(QoSClass::Maintenance);
    TEST_ASSERT_EQ(20,  (long)lvl.base_priority.value(), "Maintenance base_priority must be 20");
    TEST_ASSERT_EQ(64,  (long)lvl.quantum_ticks.value(), "Maintenance quantum must be 64 ticks");
    TEST_ASSERT_EQ(256, (long)lvl.allotment_ticks.value(), "Maintenance allotment must be 256 ticks");
    TEST_ASSERT_EQ(3,   (long)lvl.default_mlfq_level.value(), "Maintenance must map to MLFQ level 3");
    return nullptr;
}

static const char* test_qos_table_priority_ordering() {
    // UserInteractive (0) > UserInitiated (1) > ... > Maintenance (5)
    for (uint8_t i = 0; i + 1 < 6; ++i) {
        const QoSLevel& high = qos_level(static_cast<QoSClass>(i));
        const QoSLevel& low  = qos_level(static_cast<QoSClass>(i + 1));
        TEST_ASSERT(high.base_priority.value() > low.base_priority.value(),
                    "higher QoS class must have strictly higher base priority");
    }
    return nullptr;
}

static const char* test_priority_for_qos_default_nice() {
    // nice=0 → offset=0 → priority equals base (80 for Default)
    fk::TaskPriority p = priority_for_qos(QoSClass::Default);
    TEST_ASSERT_EQ(80, (long)p.value(), "Default QoS nice=0 must give priority 80");
    return nullptr;
}

static const char* test_priority_for_qos_nice_negative() {
    // nice=-20 → offset=+7 → 80+7=87
    fk::TaskPriority p = priority_for_qos(QoSClass::Default, fk::NiceValue(-20));
    TEST_ASSERT_EQ(87, (long)p.value(), "Default QoS nice=-20 must give priority 87");
    return nullptr;
}

static const char* test_priority_for_qos_nice_positive() {
    // nice=+19 → offset=-8 → 80-8=72
    fk::TaskPriority p = priority_for_qos(QoSClass::Default, fk::NiceValue(19));
    TEST_ASSERT_EQ(72, (long)p.value(), "Default QoS nice=+19 must give priority 72");
    return nullptr;
}

static const char* test_priority_for_qos_clamped_max() {
    // UserInteractive base=120; nice=-20 → offset=+7; cap = base+7=127
    fk::TaskPriority p = priority_for_qos(QoSClass::UserInteractive, fk::NiceValue(-20));
    TEST_ASSERT_EQ(127, (long)p.value(), "priority must be capped at base+7=127");
    return nullptr;
}

static const char* test_priority_for_qos_clamped_min() {
    // Maintenance base=20; nice=+19 → offset=-8; floor = base-8=12
    fk::TaskPriority p = priority_for_qos(QoSClass::Maintenance, fk::NiceValue(19));
    TEST_ASSERT_EQ(12, (long)p.value(), "priority must be floored at base-8=12");
    return nullptr;
}

static const char* test_allotment_for_qos() {
    TEST_ASSERT_EQ(8,   (long)allotment_for_qos(QoSClass::UserInteractive).value(),
                   "UserInteractive allotment must be 8");
    TEST_ASSERT_EQ(256, (long)allotment_for_qos(QoSClass::Maintenance).value(),
                   "Maintenance allotment must be 256");
    return nullptr;
}

static const char* test_quantum_for_level_all() {
    TEST_ASSERT_EQ(2,  (long)quantum_for_level(fk::MlqfLevel(0)).value(), "level 0 quantum=2");
    TEST_ASSERT_EQ(4,  (long)quantum_for_level(fk::MlqfLevel(1)).value(), "level 1 quantum=4");
    TEST_ASSERT_EQ(8,  (long)quantum_for_level(fk::MlqfLevel(2)).value(), "level 2 quantum=8");
    TEST_ASSERT_EQ(16, (long)quantum_for_level(fk::MlqfLevel(3)).value(), "level 3 quantum=16");
    return nullptr;
}

static const char* test_quantum_for_level_clamped() {
    // Any level >= 4 must clamp to the level-3 quantum (16 ticks)
    fk::TickCount clamped = quantum_for_level(fk::MlqfLevel(99));
    TEST_ASSERT_EQ(16, (long)clamped.value(), "level>=4 must clamp to level-3 quantum=16");
    return nullptr;
}

static const char* test_nice_to_priority_offset() {
    TEST_ASSERT_EQ(0,  (long)nice_to_priority_offset(fk::NiceValue(0)).value(),
                   "nice=0 must give offset=0");
    TEST_ASSERT_EQ(7,  (long)nice_to_priority_offset(fk::NiceValue(-20)).value(),
                   "nice=-20 must give offset=+7");
    TEST_ASSERT_EQ(-8, (long)nice_to_priority_offset(fk::NiceValue(19)).value(),
                   "nice=+19 must give offset=-8");
    return nullptr;
}

static const char* test_qos_from_linux_policy() {
    TEST_ASSERT(qos_from_linux_policy(0)  == QoSClass::Default,       "SCHED_OTHER(0)→Default");
    TEST_ASSERT(qos_from_linux_policy(1)  == QoSClass::UserInitiated, "SCHED_FIFO(1)→UserInitiated");
    TEST_ASSERT(qos_from_linux_policy(2)  == QoSClass::UserInitiated, "SCHED_RR(2)→UserInitiated");
    TEST_ASSERT(qos_from_linux_policy(3)  == QoSClass::Utility,       "SCHED_BATCH(3)→Utility");
    TEST_ASSERT(qos_from_linux_policy(5)  == QoSClass::Maintenance,   "SCHED_IDLE(5)→Maintenance");
    TEST_ASSERT(qos_from_linux_policy(99) == QoSClass::Default,       "unknown policy must fall back to Default");
    return nullptr;
}

static const char* test_linux_policy_from_qos() {
    TEST_ASSERT_EQ(0, (long)linux_policy_from_qos(SchedulingPolicy::Normal),      "Normal→0 (SCHED_OTHER)");
    TEST_ASSERT_EQ(1, (long)linux_policy_from_qos(SchedulingPolicy::Fifo),        "Fifo→1 (SCHED_FIFO)");
    TEST_ASSERT_EQ(2, (long)linux_policy_from_qos(SchedulingPolicy::RoundRobin),  "RoundRobin→2 (SCHED_RR)");
    TEST_ASSERT_EQ(3, (long)linux_policy_from_qos(SchedulingPolicy::Batch),       "Batch→3 (SCHED_BATCH)");
    TEST_ASSERT_EQ(5, (long)linux_policy_from_qos(SchedulingPolicy::Idle),        "Idle→5 (SCHED_IDLE)");
    return nullptr;
}

static test_case_t s_tests[] = {
    {"qos_level_user_interactive",    test_qos_level_user_interactive},
    {"qos_level_maintenance",         test_qos_level_maintenance},
    {"qos_table_priority_ordering",   test_qos_table_priority_ordering},
    {"priority_for_qos_default_nice", test_priority_for_qos_default_nice},
    {"priority_for_qos_nice_neg",     test_priority_for_qos_nice_negative},
    {"priority_for_qos_nice_pos",     test_priority_for_qos_nice_positive},
    {"priority_for_qos_clamped_max",  test_priority_for_qos_clamped_max},
    {"priority_for_qos_clamped_min",  test_priority_for_qos_clamped_min},
    {"allotment_for_qos",             test_allotment_for_qos},
    {"quantum_for_level_all",         test_quantum_for_level_all},
    {"quantum_for_level_clamped",     test_quantum_for_level_clamped},
    {"nice_to_priority_offset",       test_nice_to_priority_offset},
    {"qos_from_linux_policy",         test_qos_from_linux_policy},
    {"linux_policy_from_qos",         test_linux_policy_from_qos},
};

int run_kernel_qos_tests() {
    return run_tests("Kernel::QoS",
                     s_tests, sizeof(s_tests) / sizeof(s_tests[0]));
}
