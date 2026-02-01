#include <tests/test_framework.h>
#include <LibFK/Container/circular_buffer.h>

using namespace fk::containers;

static const char* test_buffer_basic() {
    CircularBuffer<int, 5> buffer;
    TEST_ASSERT(buffer.is_empty(), "Buffer should be empty initially");
    TEST_ASSERT_EQ(0, buffer.size(), "Size should be 0");

    buffer.enqueue(1);
    buffer.enqueue(2);
    TEST_ASSERT_EQ(2, buffer.size(), "Size should be 2");
    TEST_ASSERT_EQ(1, buffer.dequeue(), "First element should be 1");
    TEST_ASSERT_EQ(1, buffer.size(), "Size should be 1 after dequeue");

    return NULL;
}

static const char* test_buffer_wrap() {
    CircularBuffer<int, 3> buffer;
    buffer.enqueue(1);
    buffer.enqueue(2);
    buffer.enqueue(3);
    TEST_ASSERT(buffer.is_full(), "Buffer should be full");

    buffer.enqueue(4); // Should overwrite 1
    TEST_ASSERT_EQ(3, buffer.size(), "Size should still be 3");
    TEST_ASSERT_EQ(2, buffer.dequeue(), "Should dequeue 2");
    TEST_ASSERT_EQ(3, buffer.dequeue(), "Should dequeue 3");
    TEST_ASSERT_EQ(4, buffer.dequeue(), "Should dequeue 4");
    TEST_ASSERT(buffer.is_empty(), "Buffer should be empty");

    return NULL;
}

static const test_case_t libfk_tests[] = {
    {"test_buffer_basic", test_buffer_basic},
    {"test_buffer_wrap", test_buffer_wrap},
};

int run_libfk_container_tests() {
    return run_tests("LibFK Containers", libfk_tests, sizeof(libfk_tests) / sizeof(libfk_tests[0]));
}
