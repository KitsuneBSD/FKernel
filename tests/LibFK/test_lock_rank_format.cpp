#include <tests/test_framework.h>
#include <LibFK/Synchronization/lock_rank.h>
#include <LibFK/Synchronization/spinlock.h>
#include <LibFK/Algorithms/format_checked.h>

using namespace fk::synchronization;
using namespace fk::algorithms;

// ---- LockRank ordering ----------------------------------------------------

static const char* test_lock_rank_values() {
    TEST_ASSERT((uint32_t)LockRank::None      == 0,  "None==0");
    TEST_ASSERT((uint32_t)LockRank::Scheduler  < (uint32_t)LockRank::Memory,   "Scheduler<Memory");
    TEST_ASSERT((uint32_t)LockRank::Memory     < (uint32_t)LockRank::VFS,      "Memory<VFS");
    TEST_ASSERT((uint32_t)LockRank::VFS        < (uint32_t)LockRank::Process,  "VFS<Process");
    TEST_ASSERT((uint32_t)LockRank::Process    < (uint32_t)LockRank::Driver,   "Process<Driver");
    TEST_ASSERT((uint32_t)LockRank::Driver     < (uint32_t)LockRank::Max,      "Driver<Max");
    return NULL;
}

static const char* test_lock_rank_tracking() {
    // Reset global rank to None before test
    set_cpu_lock_rank(LockRank::None);
    TEST_ASSERT(current_cpu_lock_rank() == LockRank::None, "starts at None");

    set_cpu_lock_rank(LockRank::Scheduler);
    TEST_ASSERT(current_cpu_lock_rank() == LockRank::Scheduler, "rank set to Scheduler");

    set_cpu_lock_rank(LockRank::None);
    TEST_ASSERT(current_cpu_lock_rank() == LockRank::None, "rank reset to None");
    return NULL;
}

static const char* test_spinlock_ranked_basic() {
    set_cpu_lock_rank(LockRank::None);
    Spinlock lock(LockRank::Memory);
    lock.lock();
    TEST_ASSERT(lock.is_locked(), "lock should be locked");
    TEST_ASSERT(current_cpu_lock_rank() == LockRank::Memory, "rank advanced to Memory");
    lock.unlock();
    TEST_ASSERT(!lock.is_locked(), "lock should be unlocked");
    TEST_ASSERT(current_cpu_lock_rank() == LockRank::None, "rank restored to None");
    return NULL;
}

static const char* test_spinlock_unranked_no_track() {
    set_cpu_lock_rank(LockRank::None);
    Spinlock lock; // rank = None → no tracking
    lock.lock();
    TEST_ASSERT(lock.is_locked(), "unranked lock works");
    TEST_ASSERT(current_cpu_lock_rank() == LockRank::None, "unranked doesn't change rank");
    lock.unlock();
    return NULL;
}

// ---- format_checked -------------------------------------------------------

static const char* test_format_v_int() {
    char buf[64];
    format_v(buf, sizeof(buf), "val={}", 42);
    TEST_ASSERT(strcmp(buf, "val=42") == 0, "format_v int");
    return NULL;
}

static const char* test_format_v_two_args() {
    char buf[64];
    format_v(buf, sizeof(buf), "{} and {}", 10, 20);
    TEST_ASSERT(strcmp(buf, "10 and 20") == 0, "format_v two ints");
    return NULL;
}

static const char* test_format_v_string() {
    char buf[64];
    format_v(buf, sizeof(buf), "hello {}", "world");
    TEST_ASSERT(strcmp(buf, "hello world") == 0, "format_v string arg");
    return NULL;
}

static const char* test_format_v_bool() {
    char buf[64];
    format_v(buf, sizeof(buf), "{}", true);
    TEST_ASSERT(strcmp(buf, "true") == 0, "format_v bool true");
    format_v(buf, sizeof(buf), "{}", false);
    TEST_ASSERT(strcmp(buf, "false") == 0, "format_v bool false");
    return NULL;
}

static const char* test_format_v_no_placeholders() {
    char buf[64];
    format_v(buf, sizeof(buf), "no placeholders");
    TEST_ASSERT(strcmp(buf, "no placeholders") == 0, "format_v no placeholders");
    return NULL;
}

static const char* test_format_v_truncation() {
    char buf[8];
    format_v(buf, sizeof(buf), "longstring{}", "extra");
    TEST_ASSERT(buf[7] == '\0', "buffer is null-terminated on truncation");
    return NULL;
}

// ---- registration ----------------------------------------------------------

static const test_case_t lock_rank_format_tests[] = {
    {"lock_rank_values",           test_lock_rank_values},
    {"lock_rank_tracking",         test_lock_rank_tracking},
    {"spinlock_ranked_basic",      test_spinlock_ranked_basic},
    {"spinlock_unranked_no_track", test_spinlock_unranked_no_track},
    {"format_v_int",               test_format_v_int},
    {"format_v_two_args",          test_format_v_two_args},
    {"format_v_string",            test_format_v_string},
    {"format_v_bool",              test_format_v_bool},
    {"format_v_no_placeholders",   test_format_v_no_placeholders},
    {"format_v_truncation",        test_format_v_truncation},
};

int run_libfk_lock_rank_format_tests() {
    return run_tests("LibFK LockRank/Format",
                     lock_rank_format_tests,
                     sizeof(lock_rank_format_tests) / sizeof(lock_rank_format_tests[0]));
}
