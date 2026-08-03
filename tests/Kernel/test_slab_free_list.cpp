#include <tests/test_framework.h>
#include <Kernel/Memory/ObjectMemory/slab_free_list.h>

// Tests for SlabFreeList — the inner free-list of a slab cache.
// Only depends on LibFK/Types/types.h, so it compiles on the host.

static const char* test_slab_initial_empty() {
    SlabFreeList fl;
    TEST_ASSERT(fl.is_empty(), "default-constructed free list must be empty");
    TEST_ASSERT_EQ(0u, fl.count(), "default-constructed count must be 0");
    return nullptr;
}

static const char* test_slab_initialize_count() {
    alignas(16) uint8_t buf[8 * 16]; // 8 objects of 16 bytes
    SlabFreeList fl;
    fl.initialize(buf, 16, 8);
    TEST_ASSERT_EQ(8u, fl.count(), "initialize(buf,16,8) must set count=8");
    TEST_ASSERT(!fl.is_empty(), "initialized list must not be empty");
    return nullptr;
}

static const char* test_slab_pop_returns_objects() {
    alignas(16) uint8_t buf[4 * 16];
    SlabFreeList fl;
    fl.initialize(buf, 16, 4);

    void* a = fl.pop();
    void* b = fl.pop();
    void* c = fl.pop();
    void* d = fl.pop();

    TEST_ASSERT(a != nullptr, "pop 1 must not be null");
    TEST_ASSERT(b != nullptr, "pop 2 must not be null");
    TEST_ASSERT(c != nullptr, "pop 3 must not be null");
    TEST_ASSERT(d != nullptr, "pop 4 must not be null");
    TEST_ASSERT_EQ(0u, fl.count(), "count must be 0 after popping all");
    TEST_ASSERT(fl.is_empty(), "list must be empty after popping all");
    return nullptr;
}

static const char* test_slab_pop_empty_returns_null() {
    SlabFreeList fl;
    void* p = fl.pop();
    TEST_ASSERT(p == nullptr, "pop on empty list must return null");
    return nullptr;
}

static const char* test_slab_objects_in_buf() {
    alignas(32) uint8_t buf[3 * 32];
    SlabFreeList fl;
    fl.initialize(buf, 32, 3);

    void* p1 = fl.pop();
    void* p2 = fl.pop();
    void* p3 = fl.pop();

    // All popped addresses must be within the buffer.
    auto in_buf = [&](void* p) {
        uintptr_t addr = reinterpret_cast<uintptr_t>(p);
        uintptr_t base = reinterpret_cast<uintptr_t>(buf);
        return addr >= base && addr < base + 3 * 32;
    };
    TEST_ASSERT(in_buf(p1), "pop 1 must be inside buffer");
    TEST_ASSERT(in_buf(p2), "pop 2 must be inside buffer");
    TEST_ASSERT(in_buf(p3), "pop 3 must be inside buffer");

    // All popped addresses must be distinct.
    TEST_ASSERT(p1 != p2, "pop 1 != pop 2");
    TEST_ASSERT(p2 != p3, "pop 2 != pop 3");
    TEST_ASSERT(p1 != p3, "pop 1 != pop 3");
    return nullptr;
}

static const char* test_slab_push_restores_count() {
    alignas(16) uint8_t buf[2 * 16];
    SlabFreeList fl;
    fl.initialize(buf, 16, 2);

    void* a = fl.pop();
    void* b = fl.pop();
    TEST_ASSERT_EQ(0u, fl.count(), "count 0 after popping both");

    fl.push(a);
    TEST_ASSERT_EQ(1u, fl.count(), "count 1 after pushing one back");
    fl.push(b);
    TEST_ASSERT_EQ(2u, fl.count(), "count 2 after pushing both back");
    return nullptr;
}

static const char* test_slab_push_then_pop_returns_pushed() {
    alignas(8) uint8_t buf[4 * 8];
    SlabFreeList fl;
    fl.initialize(buf, 8, 4);

    // Pop all, push one back, pop it.
    fl.pop(); fl.pop(); fl.pop();
    void* last = fl.pop();
    fl.push(last);
    void* got = fl.pop();
    TEST_ASSERT_EQ(last, got, "push then pop must return the pushed object");
    return nullptr;
}

static const char* test_slab_reinitialize() {
    alignas(16) uint8_t buf[4 * 16];
    SlabFreeList fl;
    fl.initialize(buf, 16, 4);
    fl.pop(); fl.pop(); // use 2

    fl.initialize(buf, 16, 4); // reset
    TEST_ASSERT_EQ(4u, fl.count(), "re-initialize must reset count to 4");
    return nullptr;
}

static test_case_t s_tests[] = {
    {"slab_initial_empty",           test_slab_initial_empty},
    {"slab_initialize_count",        test_slab_initialize_count},
    {"slab_pop_returns_objects",     test_slab_pop_returns_objects},
    {"slab_pop_empty_returns_null",  test_slab_pop_empty_returns_null},
    {"slab_objects_in_buf",          test_slab_objects_in_buf},
    {"slab_push_restores_count",     test_slab_push_restores_count},
    {"slab_push_then_pop",           test_slab_push_then_pop_returns_pushed},
    {"slab_reinitialize",            test_slab_reinitialize},
};

int run_kernel_slab_free_list_tests() {
    return run_tests("Kernel SlabFreeList",
                     s_tests,
                     sizeof(s_tests) / sizeof(s_tests[0]));
}
