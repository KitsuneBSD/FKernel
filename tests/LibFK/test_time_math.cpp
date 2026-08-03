#include <tests/test_framework.h>
#include <LibFK/Algorithms/Generic/interval.h>
#include <LibFK/Algorithms/Generic/time_math.h>

using fk::algorithms::intervals_overlap;
using fk::algorithms::timespec_to_ticks;

static const char* test_overlap_contained() {
    TEST_ASSERT(intervals_overlap(10, 20, 12, 15), "contained overlaps");
    return nullptr;
}

static const char* test_overlap_partial() {
    TEST_ASSERT(intervals_overlap(10, 20, 15, 30), "partial overlap");
    return nullptr;
}

static const char* test_overlap_touching_inclusive() {
    TEST_ASSERT(intervals_overlap(10, 20, 20, 30), "inclusive boundary touches");
    return nullptr;
}

static const char* test_overlap_identical() {
    TEST_ASSERT(intervals_overlap(5, 5, 5, 5), "point ranges overlap");
    return nullptr;
}

static const char* test_no_overlap_before() {
    TEST_ASSERT(!intervals_overlap(20, 30, 10, 15), "b before a");
    return nullptr;
}

static const char* test_no_overlap_after() {
    TEST_ASSERT(!intervals_overlap(10, 15, 20, 30), "b after a");
    return nullptr;
}

static const char* test_ticks_zero_frequency() {
    TEST_ASSERT_EQ((long)timespec_to_ticks(1, 0, 0), 0L, "zero frequency returns 0");
    return nullptr;
}

static const char* test_ticks_seconds_only() {
    TEST_ASSERT_EQ((long)timespec_to_ticks(2, 0, 1000), 2000L, "2s at 1kHz");
    return nullptr;
}

static const char* test_ticks_nanoseconds() {
    TEST_ASSERT_EQ((long)timespec_to_ticks(0, 500000000, 1000), 500L, "0.5s at 1kHz");
    return nullptr;
}

static const char* test_ticks_combined() {
    TEST_ASSERT_EQ((long)timespec_to_ticks(1, 250000000, 1000), 1250L, "1.25s at 1kHz");
    return nullptr;
}

static const char* test_ticks_high_frequency() {
    TEST_ASSERT_EQ((long)timespec_to_ticks(0, 1000000, 250000000), 250000L, "1ms at 250MHz");
    return nullptr;
}

static const test_case_t s_tests[] = {
    {"overlap contained", test_overlap_contained},
    {"overlap partial", test_overlap_partial},
    {"overlap touching inclusive", test_overlap_touching_inclusive},
    {"overlap identical", test_overlap_identical},
    {"no overlap before", test_no_overlap_before},
    {"no overlap after", test_no_overlap_after},
    {"ticks zero frequency", test_ticks_zero_frequency},
    {"ticks seconds only", test_ticks_seconds_only},
    {"ticks nanoseconds", test_ticks_nanoseconds},
    {"ticks combined", test_ticks_combined},
    {"ticks high frequency", test_ticks_high_frequency},
};

int run_libfk_time_math_tests() {
    return run_tests("TimeMath", s_tests, sizeof(s_tests) / sizeof(s_tests[0]));
}
