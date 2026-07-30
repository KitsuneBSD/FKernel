#include <tests/test_framework.h>
#include <Kernel/Fs/Vfs/FileLock/file_lock_list.h>

using fkernel::FileLockList;
using fk::ProcessId;

static const ProcessId PID1{1};
static const ProcessId PID2{2};
static const ProcessId PID3{3};

// Helper: acquire must succeed
static const char* test_rdlock_basic() {
    FileLockList list;
    bool ok = list.try_acquire(PID1, fkernel::F_RDLCK, 0, 100, nullptr, nullptr);
    TEST_ASSERT(ok, "single read lock must succeed");
    return nullptr;
}

// Two readers are compatible
static const char* test_two_rdlocks_compatible() {
    FileLockList list;
    list.try_acquire(PID1, fkernel::F_RDLCK, 0, 100, nullptr, nullptr);
    bool ok = list.try_acquire(PID2, fkernel::F_RDLCK, 0, 100, nullptr, nullptr);
    TEST_ASSERT(ok, "two read locks on same range must be compatible");
    return nullptr;
}

// Write lock conflicts with existing write lock
static const char* test_wrlck_conflicts_with_wrlck() {
    FileLockList list;
    list.try_acquire(PID1, fkernel::F_WRLCK, 0, 100, nullptr, nullptr);
    ProcessId conflict_pid{0};
    short conflict_type = -1;
    bool ok = list.try_acquire(PID2, fkernel::F_WRLCK, 0, 100, &conflict_pid, &conflict_type);
    TEST_ASSERT(!ok, "write lock must conflict with existing write lock");
    TEST_ASSERT(conflict_pid == PID1, "conflict pid must be PID1");
    TEST_ASSERT_EQ(fkernel::F_WRLCK, conflict_type, "conflict type must be F_WRLCK");
    return nullptr;
}

// Write lock conflicts with existing read lock
static const char* test_wrlck_conflicts_with_rdlck() {
    FileLockList list;
    list.try_acquire(PID1, fkernel::F_RDLCK, 0, 100, nullptr, nullptr);
    bool ok = list.try_acquire(PID2, fkernel::F_WRLCK, 0, 100, nullptr, nullptr);
    TEST_ASSERT(!ok, "write lock must conflict with existing read lock");
    return nullptr;
}

// Read lock conflicts with existing write lock
static const char* test_rdlck_conflicts_with_wrlck() {
    FileLockList list;
    list.try_acquire(PID1, fkernel::F_WRLCK, 0, 100, nullptr, nullptr);
    bool ok = list.try_acquire(PID2, fkernel::F_RDLCK, 0, 100, nullptr, nullptr);
    TEST_ASSERT(!ok, "read lock must conflict with existing write lock");
    return nullptr;
}

// Non-overlapping ranges do not conflict
static const char* test_non_overlapping_no_conflict() {
    FileLockList list;
    list.try_acquire(PID1, fkernel::F_WRLCK, 0, 49, nullptr, nullptr);
    bool ok = list.try_acquire(PID2, fkernel::F_WRLCK, 50, 100, nullptr, nullptr);
    TEST_ASSERT(ok, "non-overlapping write locks must not conflict");
    return nullptr;
}

// Release removes the lock, allowing a new conflicting lock
static const char* test_release_removes_lock() {
    FileLockList list;
    list.try_acquire(PID1, fkernel::F_WRLCK, 0, 100, nullptr, nullptr);
    list.release(PID1, 0, 100);
    bool ok = list.try_acquire(PID2, fkernel::F_WRLCK, 0, 100, nullptr, nullptr);
    TEST_ASSERT(ok, "after release, conflicting lock must now succeed");
    return nullptr;
}

// release_all_for_process clears all locks for a PID
static const char* test_release_all_for_process() {
    FileLockList list;
    list.try_acquire(PID1, fkernel::F_RDLCK, 0, 50, nullptr, nullptr);
    list.try_acquire(PID1, fkernel::F_RDLCK, 60, 100, nullptr, nullptr);
    list.try_acquire(PID2, fkernel::F_RDLCK, 0, 50, nullptr, nullptr);
    list.release_all_for_process(PID1);

    // PID2's lock must still block a write from PID3
    bool ok = list.try_acquire(PID3, fkernel::F_WRLCK, 0, 50, nullptr, nullptr);
    TEST_ASSERT(!ok, "PID2 read lock must still block write after PID1 released");

    // PID1's second range must now be free
    ok = list.try_acquire(PID3, fkernel::F_WRLCK, 60, 100, nullptr, nullptr);
    TEST_ASSERT(ok, "range 60-100 must be free after PID1 released all");
    return nullptr;
}

// test_conflict does not acquire
static const char* test_conflict_non_acquiring() {
    FileLockList list;
    list.try_acquire(PID1, fkernel::F_WRLCK, 0, 100, nullptr, nullptr);

    ProcessId cpid{0};
    short ctype = -1;
    bool conflict = list.test_conflict(fkernel::F_RDLCK, 0, 100, &cpid, &ctype);
    TEST_ASSERT(conflict, "test_conflict must detect write lock");
    TEST_ASSERT(cpid == PID1, "conflict pid must be PID1");

    // After test_conflict, a second test_conflict must still detect (was not acquired)
    conflict = list.test_conflict(fkernel::F_WRLCK, 0, 100, nullptr, nullptr);
    TEST_ASSERT(conflict, "test_conflict must be idempotent (no acquisition)");
    return nullptr;
}

// Adjacent ranges: boundary at l_end inclusive
static const char* test_boundary_adjacent_ranges() {
    FileLockList list;
    // Lock [0, 49] inclusive
    list.try_acquire(PID1, fkernel::F_WRLCK, 0, 49, nullptr, nullptr);
    // [50, 100] must not overlap [0, 49]
    bool ok = list.try_acquire(PID2, fkernel::F_WRLCK, 50, 100, nullptr, nullptr);
    TEST_ASSERT(ok, "range [50,100] must not conflict with [0,49]");
    return nullptr;
}

// Swap-and-pop correctness: release_all preserves remaining entries
static const char* test_release_all_swap_pop_correctness() {
    FileLockList list;
    // Insert 5 entries for PID1, 1 for PID2 in the middle position (logically)
    list.try_acquire(PID1, fkernel::F_RDLCK,  0,  9, nullptr, nullptr);
    list.try_acquire(PID1, fkernel::F_RDLCK, 10, 19, nullptr, nullptr);
    list.try_acquire(PID2, fkernel::F_RDLCK, 20, 29, nullptr, nullptr);
    list.try_acquire(PID1, fkernel::F_RDLCK, 30, 39, nullptr, nullptr);
    list.try_acquire(PID1, fkernel::F_RDLCK, 40, 49, nullptr, nullptr);
    list.release_all_for_process(PID1);

    // PID2's lock on [20,29] must still block a write
    bool ok = list.try_acquire(PID3, fkernel::F_WRLCK, 20, 29, nullptr, nullptr);
    TEST_ASSERT(!ok, "PID2 lock must survive release_all for PID1");

    // All PID1 ranges must now be free
    ok = list.try_acquire(PID3, fkernel::F_WRLCK, 0, 19, nullptr, nullptr);
    TEST_ASSERT(ok, "PID1 range [0,19] must be free after release_all");
    return nullptr;
}

int run_kernel_file_lock_tests() {
    static const test_case_t tests[] = {
        {"rdlock_basic",               test_rdlock_basic},
        {"two_rdlocks_compatible",     test_two_rdlocks_compatible},
        {"wrlck_conflicts_wrlck",      test_wrlck_conflicts_with_wrlck},
        {"wrlck_conflicts_rdlck",      test_wrlck_conflicts_with_rdlck},
        {"rdlck_conflicts_wrlck",      test_rdlck_conflicts_with_wrlck},
        {"non_overlapping_no_conflict",test_non_overlapping_no_conflict},
        {"release_removes_lock",       test_release_removes_lock},
        {"release_all_for_process",    test_release_all_for_process},
        {"test_conflict_non_acquiring",test_conflict_non_acquiring},
        {"boundary_adjacent_ranges",   test_boundary_adjacent_ranges},
        {"release_all_swap_pop",       test_release_all_swap_pop_correctness},
    };
    return run_tests("Kernel::FileLockList", tests,
                     sizeof(tests) / sizeof(tests[0]));
}
