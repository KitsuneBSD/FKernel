#include <tests/test_framework.h>
#include <LibFK/Algorithms/internet_checksum.h>
#include <LibFK/Algorithms/binary_search.h>
#include <LibFK/Algorithms/container_algorithms.h>
#include <LibFK/Algorithms/gather.h>
#include <LibFK/Core/result.h>
#include <LibFK/Functional/function.h>

using namespace fk::algorithms;
using namespace fk::core;
using namespace fk::functional;

// ---------------------------------------------------------------------------
// InternetChecksum
// ---------------------------------------------------------------------------

static const char* test_inet_cksum_all_zeros() {
    uint8_t buf[4] = {0x00, 0x00, 0x00, 0x00};
    uint16_t ck = InternetChecksum::compute(buf, 4);
    TEST_ASSERT_EQ(0xFFFF, (int)(unsigned)ck, "all-zeros buffer checksum should be 0xFFFF");
    return NULL;
}

static const char* test_inet_cksum_all_ones() {
    uint8_t buf[2] = {0xFF, 0xFF};
    uint16_t ck = InternetChecksum::compute(buf, 2);
    TEST_ASSERT_EQ(0x0000, (int)(unsigned)ck, "all-ones 16-bit word checksum should be 0x0000");
    return NULL;
}

static const char* test_inet_cksum_odd_byte() {
    uint8_t buf[1] = {0x01};
    uint16_t ck = InternetChecksum::compute(buf, 1);
    TEST_ASSERT_EQ(0xFFFE, (int)(unsigned)ck, "single byte 0x01 checksum should be 0xFFFE");
    return NULL;
}

static const char* test_inet_cksum_verify() {
    // Compute checksum of a 4-byte buffer, then append it and verify
    uint8_t buf[4] = {0x01, 0x02, 0x03, 0x04};
    uint16_t ck = InternetChecksum::compute(buf, 4);

    uint8_t buf2[6];
    __builtin_memcpy(buf2, buf, 4);
    buf2[4] = (uint8_t)(ck & 0xFF);
    buf2[5] = (uint8_t)(ck >> 8);

    uint16_t verify = InternetChecksum::compute(buf2, 6);
    TEST_ASSERT_EQ(0x0000, (int)(unsigned)verify, "verification of buffer+checksum should be 0x0000");
    return NULL;
}

static const char* test_inet_cksum_multi_region() {
    // Accumulate two regions; should equal single compute over combined buffer
    uint8_t a[4] = {0x01, 0x02, 0x03, 0x04};
    uint8_t b[2] = {0x05, 0x06};
    uint8_t combined[6];
    __builtin_memcpy(combined, a, 4);
    __builtin_memcpy(combined + 4, b, 2);

    InternetChecksum ic;
    ic.accumulate(a, 4);
    ic.accumulate(b, 2);
    uint16_t multi = ic.finalize();
    uint16_t single = InternetChecksum::compute(combined, 6);

    TEST_ASSERT_EQ((int)(unsigned)single, (int)(unsigned)multi, "multi-region should match single-buffer");
    return NULL;
}

// ---------------------------------------------------------------------------
// BinarySearch
// ---------------------------------------------------------------------------

static const char* test_lower_bound_basic() {
    int arr[] = {1, 2, 4, 5, 8};
    TEST_ASSERT_EQ(2, (int)lower_bound(arr, 5, 4), "lower_bound(4) in {1,2,4,5,8}");
    TEST_ASSERT_EQ(0, (int)lower_bound(arr, 5, 0), "lower_bound(0) before all");
    TEST_ASSERT_EQ(5, (int)lower_bound(arr, 5, 9), "lower_bound(9) after all");
    TEST_ASSERT_EQ(2, (int)lower_bound(arr, 5, 3), "lower_bound(3) between 2 and 4");
    return NULL;
}

static const char* test_upper_bound_basic() {
    int arr[] = {1, 2, 4, 5, 8};
    TEST_ASSERT_EQ(3, (int)upper_bound(arr, 5, 4), "upper_bound(4) in {1,2,4,5,8}");
    TEST_ASSERT_EQ(0, (int)upper_bound(arr, 5, 0), "upper_bound(0) before all");
    TEST_ASSERT_EQ(5, (int)upper_bound(arr, 5, 8), "upper_bound(8) after all");
    TEST_ASSERT_EQ(2, (int)upper_bound(arr, 5, 3), "upper_bound(3) between 2 and 4");
    return NULL;
}

static const char* test_bounds_empty() {
    int arr[] = {42};
    TEST_ASSERT_EQ(0, (int)lower_bound(arr, 0, 42), "lower_bound on empty array");
    TEST_ASSERT_EQ(0, (int)upper_bound(arr, 0, 42), "upper_bound on empty array");
    return NULL;
}

// ---------------------------------------------------------------------------
// ContainerAlgorithms
// ---------------------------------------------------------------------------

static const char* test_find_if_found() {
    int arr[] = {10, 20, 30, 40};
    size_t idx = find_if(arr, 4, [](const int& v) { return v == 30; });
    TEST_ASSERT_EQ(2, (int)idx, "find_if should return index 2 for value 30");
    return NULL;
}

static const char* test_find_if_not_found() {
    int arr[] = {10, 20, 30};
    size_t idx = find_if(arr, 3, [](const int& v) { return v == 99; });
    TEST_ASSERT_EQ(3, (int)idx, "find_if should return size when not found");
    return NULL;
}

static const char* test_swap_remove_middle() {
    int arr[] = {1, 2, 3, 4, 5};
    size_t sz = 5;
    swap_remove(arr, sz, 2);
    TEST_ASSERT_EQ(4, (int)sz, "size should decrease after swap_remove");
    TEST_ASSERT_EQ(5, arr[2], "element at removed slot should be last element (5)");
    return NULL;
}

static const char* test_swap_remove_last() {
    int arr[] = {1, 2, 3};
    size_t sz = 3;
    swap_remove(arr, sz, 2);
    TEST_ASSERT_EQ(2, (int)sz, "size should decrease after removing last");
    TEST_ASSERT_EQ(1, arr[0], "first element unchanged");
    TEST_ASSERT_EQ(2, arr[1], "second element unchanged");
    return NULL;
}

static const char* test_find_and_remove_found() {
    int arr[] = {10, 20, 30, 40};
    size_t sz = 4;
    bool removed = find_and_remove(arr, sz, [](const int& v) { return v == 20; });
    TEST_ASSERT(removed, "find_and_remove should return true when found");
    TEST_ASSERT_EQ(3, (int)sz, "size should decrease by 1");
    return NULL;
}

static const char* test_find_and_remove_not_found() {
    int arr[] = {10, 20, 30};
    size_t sz = 3;
    bool removed = find_and_remove(arr, sz, [](const int& v) { return v == 99; });
    TEST_ASSERT(!removed, "find_and_remove should return false when not found");
    TEST_ASSERT_EQ(3, (int)sz, "size should be unchanged");
    return NULL;
}

// ---------------------------------------------------------------------------
// Gather
// ---------------------------------------------------------------------------

static const char* test_gather_copy_basic() {
    const char seg1[] = "hello";
    const char seg2[] = " world";
    IoVec iov[2] = {
        {seg1, 5},
        {seg2, 6},
    };
    uint8_t out[12] = {};
    size_t written = gather_copy(iov, 2, out);
    TEST_ASSERT_EQ(11, (int)written, "gather_copy should return total bytes written");
    TEST_ASSERT(__builtin_memcmp(out, "hello world", 11) == 0, "gathered output should match");
    return NULL;
}

static const char* test_gather_total() {
    IoVec iov[3] = {
        {(const void*)"a", 10},
        {(const void*)"b", 5},
        {(const void*)"c", 3},
    };
    TEST_ASSERT_EQ(18, (int)gather_total(iov, 3), "gather_total should sum lengths");
    return NULL;
}

static const char* test_gather_skip_null() {
    const char data[] = "data";
    IoVec iov[3] = {
        {nullptr, 4},
        {data, 4},
        {nullptr, 0},
    };
    uint8_t out[8] = {};
    size_t written = gather_copy(iov, 3, out);
    TEST_ASSERT_EQ(4, (int)written, "gather_copy skips null segments");
    TEST_ASSERT(__builtin_memcmp(out, "data", 4) == 0, "output should be 'data'");
    return NULL;
}

// ---------------------------------------------------------------------------
// Result<T, E>
// ---------------------------------------------------------------------------

static const char* test_result_ok() {
    Result<int> r(42);
    TEST_ASSERT(r.is_ok(), "Result with value should be ok");
    TEST_ASSERT(!r.is_error(), "Result with value should not be error");
    TEST_ASSERT_EQ(42, r.value(), "Result value should be 42");
    return NULL;
}

static const char* test_result_error() {
    Result<int> r(Error::NotFound);
    TEST_ASSERT(!r.is_ok(), "Result with error should not be ok");
    TEST_ASSERT(r.is_error(), "Result with error should be error");
    TEST_ASSERT(r.error() == Error::NotFound, "Error should be NotFound");
    return NULL;
}

static const char* test_result_void_ok() {
    Result<void> r;
    TEST_ASSERT(r.is_ok(), "void Result default ctor should be ok");
    return NULL;
}

static const char* test_result_void_error() {
    Result<void> r(Error::OutOfMemory);
    TEST_ASSERT(r.is_error(), "void Result with error should be error");
    TEST_ASSERT(r.error() == Error::OutOfMemory, "error should be OutOfMemory");
    return NULL;
}

static const char* test_result_bool_conversion() {
    Result<int> ok(1);
    Result<int> err(Error::IOError);
    TEST_ASSERT((bool)ok, "ok result should be truthy");
    TEST_ASSERT(!(bool)err, "error result should be falsy");
    return NULL;
}

// ---------------------------------------------------------------------------
// Function<R(Args...)>
// ---------------------------------------------------------------------------

static const char* test_function_basic_lambda() {
    Function<int(int, int)> f([](int a, int b) { return a + b; });
    TEST_ASSERT_EQ(7, f(3, 4), "lambda add(3,4) should be 7");
    return NULL;
}

static const char* test_function_capture() {
    int multiplier = 5;
    Function<int(int)> f([multiplier](int x) { return x * multiplier; });
    TEST_ASSERT_EQ(20, f(4), "captured lambda 4*5 should be 20");
    return NULL;
}

static const char* test_function_copy() {
    int counter = 0;
    Function<void()> f([&counter]() { counter++; });
    Function<void()> g = f;
    f();
    g();
    TEST_ASSERT_EQ(2, counter, "both original and copy should increment counter");
    return NULL;
}

static const char* test_function_move() {
    Function<int()> f([]() { return 42; });
    Function<int()> g = static_cast<Function<int()>&&>(f);
    TEST_ASSERT_EQ(42, g(), "moved function should still be callable");
    return NULL;
}

static const char* test_function_empty_default() {
    Function<int()> f;
    TEST_ASSERT(!f, "default-constructed Function should be empty/false");
    return NULL;
}

static const char* test_function_bool_conversion() {
    Function<void()> f([]() {});
    TEST_ASSERT((bool)f, "assigned Function should be truthy");
    return NULL;
}

// ---------------------------------------------------------------------------
// Test suite registration
// ---------------------------------------------------------------------------

static const test_case_t s_algorithm_tests[] = {
    {"inet_cksum_all_zeros",        test_inet_cksum_all_zeros},
    {"inet_cksum_all_ones",         test_inet_cksum_all_ones},
    {"inet_cksum_odd_byte",         test_inet_cksum_odd_byte},
    {"inet_cksum_verify",           test_inet_cksum_verify},
    {"inet_cksum_multi_region",     test_inet_cksum_multi_region},
    {"lower_bound_basic",           test_lower_bound_basic},
    {"upper_bound_basic",           test_upper_bound_basic},
    {"bounds_empty",                test_bounds_empty},
    {"find_if_found",               test_find_if_found},
    {"find_if_not_found",           test_find_if_not_found},
    {"swap_remove_middle",          test_swap_remove_middle},
    {"swap_remove_last",            test_swap_remove_last},
    {"find_and_remove_found",       test_find_and_remove_found},
    {"find_and_remove_not_found",   test_find_and_remove_not_found},
    {"gather_copy_basic",           test_gather_copy_basic},
    {"gather_total",                test_gather_total},
    {"gather_skip_null",            test_gather_skip_null},
    {"result_ok",                   test_result_ok},
    {"result_error",                test_result_error},
    {"result_void_ok",              test_result_void_ok},
    {"result_void_error",           test_result_void_error},
    {"result_bool_conversion",      test_result_bool_conversion},
    {"function_basic_lambda",       test_function_basic_lambda},
    {"function_capture",            test_function_capture},
    {"function_copy",               test_function_copy},
    {"function_move",               test_function_move},
    {"function_empty_default",      test_function_empty_default},
    {"function_bool_conversion",    test_function_bool_conversion},
};

int run_libfk_algorithm_tests() {
    return run_tests("LibFK Algorithms/Core",
                     s_algorithm_tests,
                     sizeof(s_algorithm_tests) / sizeof(s_algorithm_tests[0]));
}
