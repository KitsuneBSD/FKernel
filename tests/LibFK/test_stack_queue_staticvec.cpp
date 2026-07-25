#include <tests/test_framework.h>
#include <LibFK/Container/stack.h>
#include <LibFK/Container/queue.h>
#include <LibFK/Container/static_vector.h>
#include <LibFK/Container/list.h>
#include <LibFK/Container/span.h>
#include <LibFK/Container/array.h>

using namespace fk::containers;

/* ---- Test struct for intrusive containers ---- */

struct TestItem {
    int value;
    IntrusiveListNode<TestItem> m_list_node;
    TestItem(int v = 0) : value(v) {}
};

/* ---- Stack ---- */

static const char* test_stack_push_pop() {
    Stack<int, 8> s;
    TEST_ASSERT_EQ(0u, s.size(), "Stack should be empty initially");
    TEST_ASSERT(s.push(10), "Push 10 should succeed");
    TEST_ASSERT(s.push(20), "Push 20 should succeed");
    TEST_ASSERT(s.push(30), "Push 30 should succeed");
    TEST_ASSERT_EQ(3u, s.size(), "Size should be 3");

    int out = 0;
    TEST_ASSERT(s.pop(out), "Pop should succeed");
    TEST_ASSERT_EQ(30, out, "Pop should return 30 (LIFO)");
    TEST_ASSERT(s.pop(out), "Pop should succeed");
    TEST_ASSERT_EQ(20, out, "Pop should return 20");
    TEST_ASSERT(s.pop(out), "Pop should succeed");
    TEST_ASSERT_EQ(10, out, "Pop should return 10");
    TEST_ASSERT_EQ(0u, s.size(), "Stack should be empty after popping all");
    return NULL;
}

static const char* test_stack_peek() {
    Stack<int, 4> s;
    s.push(42);
    s.push(99);
    int out = 0;
    TEST_ASSERT(s.peek(out), "Peek should succeed");
    TEST_ASSERT_EQ(99, out, "Peek should return top (99) without removing");
    TEST_ASSERT_EQ(2u, s.size(), "Size should still be 2 after peek");
    return NULL;
}

static const char* test_stack_overflow() {
    Stack<int, 3> s;
    TEST_ASSERT(s.push(1), "Push 1 should succeed");
    TEST_ASSERT(s.push(2), "Push 2 should succeed");
    TEST_ASSERT(s.push(3), "Push 3 should succeed");
    TEST_ASSERT(!s.push(4), "Push on full stack should fail");
    TEST_ASSERT_EQ(3u, s.size(), "Size should still be 3");
    return NULL;
}

static const char* test_stack_underflow() {
    Stack<int, 4> s;
    int out = 0;
    TEST_ASSERT(!s.pop(out), "Pop on empty stack should fail");
    TEST_ASSERT(!s.peek(out), "Peek on empty stack should fail");
    return NULL;
}

static const char* test_stack_clear() {
    Stack<int, 8> s;
    s.push(1); s.push(2); s.push(3);
    s.clear();
    TEST_ASSERT_EQ(0u, s.size(), "Size should be 0 after clear");
    int out = 0;
    TEST_ASSERT(!s.pop(out), "Pop after clear should fail");
    return NULL;
}

static const char* test_stack_capacity() {
    Stack<int, 16> s;
    TEST_ASSERT_EQ(16u, s.capacity(), "Capacity should be 16");
    TEST_ASSERT_EQ(0u, s.size(), "Initial size should be 0");
    return NULL;
}

/* ---- Queue (intrusive) ---- */

static const char* test_queue_enqueue_dequeue() {
    Queue<TestItem> q;
    TEST_ASSERT(q.is_empty(), "Queue should be empty initially");

    TestItem a(1), b(2), c(3);
    q.enqueue(&a);
    q.enqueue(&b);
    q.enqueue(&c);
    TEST_ASSERT_EQ(3u, q.size(), "Size should be 3");

    TEST_ASSERT_EQ(&a, q.front(), "Front should be item 1");
    TEST_ASSERT_EQ(&c, q.back(), "Back should be item 3");

    TestItem* out = q.dequeue();
    TEST_ASSERT_EQ(&a, out, "Dequeue should return item 1 (FIFO)");
    out = q.dequeue();
    TEST_ASSERT_EQ(&b, out, "Dequeue should return item 2");
    out = q.dequeue();
    TEST_ASSERT_EQ(&c, out, "Dequeue should return item 3");
    TEST_ASSERT(q.is_empty(), "Queue should be empty after draining");
    return NULL;
}

static const char* test_queue_peek() {
    Queue<TestItem> q;
    TestItem a(10), b(20);
    q.enqueue(&a);
    q.enqueue(&b);

    const TestItem* front = q.front();
    TEST_ASSERT_NOT_NULL(front, "front() should not be null");
    TEST_ASSERT_EQ(10, front->value, "Front value should be 10");

    const TestItem* back = q.back();
    TEST_ASSERT_NOT_NULL(back, "back() should not be null");
    TEST_ASSERT_EQ(20, back->value, "Back value should be 20");
    return NULL;
}

static const char* test_queue_clear() {
    Queue<TestItem> q;
    TestItem a(1), b(2), c(3);
    q.enqueue(&a);
    q.enqueue(&b);
    q.enqueue(&c);
    q.clear();
    TEST_ASSERT(q.is_empty(), "Queue should be empty after clear");
    TEST_ASSERT_EQ(0u, q.size(), "Size should be 0 after clear");
    return NULL;
}

static const char* test_queue_nullptr_enqueue() {
    Queue<TestItem> q;
    q.enqueue(nullptr);
    TEST_ASSERT(q.is_empty(), "Enqueue nullptr should be a no-op");
    TEST_ASSERT_EQ(0u, q.size(), "Size should still be 0");
    return NULL;
}

/* ---- static_vector ---- */

static const char* test_staticvec_push_pop() {
    static_vector<int, 8> sv;
    TEST_ASSERT_EQ(0u, sv.size(), "Size should be 0 initially");
    TEST_ASSERT_EQ(8u, sv.capacity(), "Capacity should be 8");

    TEST_ASSERT(sv.push_back(10), "Push 10 should succeed");
    TEST_ASSERT(sv.push_back(20), "Push 20 should succeed");
    TEST_ASSERT(sv.push_back(30), "Push 30 should succeed");
    TEST_ASSERT_EQ(3u, sv.size(), "Size should be 3");
    TEST_ASSERT_EQ(10, sv[0], "Element 0 should be 10");
    TEST_ASSERT_EQ(20, sv[1], "Element 1 should be 20");
    TEST_ASSERT_EQ(30, sv[2], "Element 2 should be 30");
    return NULL;
}

static const char* test_staticvec_overflow() {
    static_vector<int, 3> sv;
    TEST_ASSERT(sv.push_back(1), "Push 1 should succeed");
    TEST_ASSERT(sv.push_back(2), "Push 2 should succeed");
    TEST_ASSERT(sv.push_back(3), "Push 3 should succeed");
    TEST_ASSERT(!sv.push_back(4), "Push on full static_vector should fail");
    TEST_ASSERT_EQ(3u, sv.size(), "Size should still be 3");
    TEST_ASSERT(sv.is_full(), "Should report is_full");
    return NULL;
}

static const char* test_staticvec_front_back() {
    static_vector<int, 4> sv;
    sv.push_back(10);
    sv.push_back(20);
    sv.push_back(30);
    TEST_ASSERT_EQ(10, sv.front(), "front() should be 10");
    TEST_ASSERT_EQ(30, sv.back(), "back() should be 30");
    return NULL;
}

static const char* test_staticvec_erase() {
    static_vector<int, 8> sv;
    sv.push_back(1);
    sv.push_back(2);
    sv.push_back(3);
    sv.push_back(4);
    sv.erase(1); // remove element at index 1 (value 2)
    TEST_ASSERT_EQ(3u, sv.size(), "Size should be 3 after erase");
    TEST_ASSERT_EQ(1, sv[0], "Element 0 should be 1");
    TEST_ASSERT_EQ(3, sv[1], "Element 1 should be 3 (shifted)");
    TEST_ASSERT_EQ(4, sv[2], "Element 2 should be 4 (shifted)");
    return NULL;
}

static const char* test_staticvec_clear() {
    static_vector<int, 4> sv;
    sv.push_back(1); sv.push_back(2);
    sv.clear();
    TEST_ASSERT_EQ(0u, sv.size(), "Size should be 0 after clear");
    return NULL;
}

static const char* test_staticvec_iterator() {
    static_vector<int, 4> sv;
    sv.push_back(10);
    sv.push_back(20);
    sv.push_back(30);
    int sum = 0;
    for (auto& x : sv) sum += x;
    TEST_ASSERT_EQ(60, sum, "Sum via iterator should be 60");
    return NULL;
}

static const char* test_staticvec_copy_push() {
    static_vector<int, 4> sv;
    int val = 42;
    sv.push_back(val); // lvalue push_back
    TEST_ASSERT_EQ(1u, sv.size(), "Size should be 1");
    TEST_ASSERT_EQ(42, sv[0], "Element should be 42");
    return NULL;
}

/* ---- List (intrusive) ---- */

static const char* test_list_push_back_front() {
    List<TestItem> list;
    TEST_ASSERT(list.empty(), "List should be empty initially");

    TestItem a(1), b(2), c(3);
    list.push_back(&a);
    list.push_front(&c);
    list.push_back(&b);

    TEST_ASSERT_EQ(3u, list.size(), "Size should be 3");
    TEST_ASSERT_EQ(&c, list.front(), "Front should be item 3");
    TEST_ASSERT_EQ(&b, list.back(), "Back should be item 2");
    return NULL;
}

static const char* test_list_remove() {
    List<TestItem> list;
    TestItem a(1), b(2), c(3);
    list.push_back(&a);
    list.push_back(&b);
    list.push_back(&c);

    list.remove(&b);
    TEST_ASSERT_EQ(2u, list.size(), "Size should be 2 after removing middle");
    TEST_ASSERT_EQ(&a, list.front(), "Front should be item 1");
    TEST_ASSERT_EQ(&c, list.back(), "Back should be item 3");

    list.remove(&a);
    TEST_ASSERT_EQ(1u, list.size(), "Size should be 1");
    TEST_ASSERT_EQ(&c, list.front(), "Front should be item 3");

    list.remove(&c);
    TEST_ASSERT(list.empty(), "List should be empty");
    return NULL;
}

static const char* test_list_pop_front_back() {
    List<TestItem> list;
    TestItem a(1), b(2), c(3);
    list.push_back(&a);
    list.push_back(&b);
    list.push_back(&c);

    TestItem* out = list.pop_front();
    TEST_ASSERT_EQ(&a, out, "pop_front should return item 1");
    TEST_ASSERT_EQ(2u, list.size(), "Size should be 2");

    out = list.pop_back();
    TEST_ASSERT_EQ(&c, out, "pop_back should return item 3");
    TEST_ASSERT_EQ(1u, list.size(), "Size should be 1");

    out = list.pop_front();
    TEST_ASSERT_EQ(&b, out, "pop_front should return item 2");
    TEST_ASSERT(list.empty(), "List should be empty");
    return NULL;
}

static const char* test_list_clear() {
    List<TestItem> list;
    TestItem a(1), b(2), c(3);
    list.push_back(&a);
    list.push_back(&b);
    list.push_back(&c);
    list.clear();
    TEST_ASSERT(list.empty(), "List should be empty after clear");
    TEST_ASSERT_EQ(0u, list.size(), "Size should be 0");
    // Verify nodes are reset (can re-add)
    list.push_back(&a);
    TEST_ASSERT_EQ(1u, list.size(), "Should be able to re-add after clear");
    return NULL;
}

static const char* test_list_iterator() {
    List<TestItem> list;
    TestItem a(1), b(2), c(3);
    list.push_back(&a);
    list.push_back(&b);
    list.push_back(&c);

    int expected = 1;
    for (auto& item : list) {
        TEST_ASSERT_EQ(expected, item.value, "Iterator should visit in order");
        expected++;
    }
    TEST_ASSERT_EQ(4, expected, "Iterator should have visited all 3 items");
    return NULL;
}

static const char* test_list_nullptr_ops() {
    List<TestItem> list;
    list.push_back(nullptr);
    TEST_ASSERT(list.empty(), "Push nullptr should be no-op");
    list.remove(nullptr);
    TestItem* out = list.pop_front();
    TEST_ASSERT_EQ(nullptr, out, "pop_front on empty should return nullptr");
    out = list.pop_back();
    TEST_ASSERT_EQ(nullptr, out, "pop_back on empty should return nullptr");
    return NULL;
}

/* ---- span ---- */

static const char* test_span_basic() {
    int arr[] = {10, 20, 30, 40, 50};
    span<int> s(arr, 5);
    TEST_ASSERT_EQ(5u, s.size(), "Size should be 5");
    TEST_ASSERT_EQ(10, s[0], "Element 0 should be 10");
    TEST_ASSERT_EQ(30, s[2], "Element 2 should be 30");
    TEST_ASSERT_EQ(50, s[4], "Element 4 should be 50");
    return NULL;
}

static const char* test_span_default_empty() {
    span<int> s;
    TEST_ASSERT_EQ(0u, s.size(), "Default span should be empty");
    return NULL;
}

static const char* test_span_mutate() {
    int arr[] = {1, 2, 3};
    span<int> s(arr, 3);
    s[1] = 99;
    TEST_ASSERT_EQ(99, arr[1], "Span mutation should affect underlying array");
    return NULL;
}

static const char* test_span_iterator() {
    int arr[] = {1, 2, 3, 4, 5};
    span<int> s(arr, 5);
    int sum = 0;
    for (auto& x : s) sum += x;
    TEST_ASSERT_EQ(15, sum, "Sum via iterator should be 15");
    return NULL;
}

static const char* test_span_const() {
    int arr[] = {10, 20, 30};
    const span<int> s(arr, 3);
    TEST_ASSERT_EQ(10, s[0], "Const span element 0 should be 10");
    TEST_ASSERT_EQ(30, s[2], "Const span element 2 should be 30");
    return NULL;
}

/* ---- array ---- */

static const char* test_array_size() {
    array<int, 5> a;
    TEST_ASSERT_EQ(5u, a.size(), "Size should be 5");
    TEST_ASSERT(!a.empty(), "Should not be empty");
    return NULL;
}

static const char* test_array_access() {
    array<int, 3> a;
    a[0] = 10;
    a[1] = 20;
    a[2] = 30;
    TEST_ASSERT_EQ(10, a[0], "Element 0 should be 10");
    TEST_ASSERT_EQ(20, a[1], "Element 1 should be 20");
    TEST_ASSERT_EQ(30, a[2], "Element 2 should be 30");
    return NULL;
}

static const char* test_array_front_back() {
    array<int, 4> a;
    a[0] = 1;
    a[3] = 4;
    TEST_ASSERT_EQ(1, a.front(), "front() should be 1");
    TEST_ASSERT_EQ(4, a.back(), "back() should be 4");
    return NULL;
}

static const char* test_array_fill() {
    array<int, 8> a;
    a.fill(42);
    for (size_t i = 0; i < 8; ++i) {
        TEST_ASSERT_EQ(42, a[i], "All elements should be 42 after fill");
    }
    return NULL;
}

static const char* test_array_iterator() {
    array<int, 4> a;
    a[0] = 1; a[1] = 2; a[2] = 3; a[3] = 4;
    int sum = 0;
    for (auto& x : a) sum += x;
    TEST_ASSERT_EQ(10, sum, "Sum via iterator should be 10");
    return NULL;
}

static const char* test_array_data() {
    array<int, 3> a;
    a[0] = 5; a[1] = 10; a[2] = 15;
    int* p = a.data();
    TEST_ASSERT_EQ(5, p[0], "data()[0] should be 5");
    TEST_ASSERT_EQ(15, p[2], "data()[2] should be 15");
    return NULL;
}

static const char* test_array_const_access() {
    array<int, 3> a;
    a[0] = 100;
    const array<int, 3>& ca = a;
    TEST_ASSERT_EQ(100, ca[0], "Const access should work");
    TEST_ASSERT_EQ(100, ca.front(), "Const front() should work");
    return NULL;
}

/* ---- Registration ---- */

static test_case_t s_tests[] = {
    // Stack
    {"test_stack_push_pop",             test_stack_push_pop},
    {"test_stack_peek",                 test_stack_peek},
    {"test_stack_overflow",             test_stack_overflow},
    {"test_stack_underflow",            test_stack_underflow},
    {"test_stack_clear",                test_stack_clear},
    {"test_stack_capacity",             test_stack_capacity},
    // Queue
    {"test_queue_enqueue_dequeue",      test_queue_enqueue_dequeue},
    {"test_queue_peek",                 test_queue_peek},
    {"test_queue_clear",                test_queue_clear},
    {"test_queue_nullptr_enqueue",      test_queue_nullptr_enqueue},
    // static_vector
    {"test_staticvec_push_pop",         test_staticvec_push_pop},
    {"test_staticvec_overflow",         test_staticvec_overflow},
    {"test_staticvec_front_back",       test_staticvec_front_back},
    {"test_staticvec_erase",            test_staticvec_erase},
    {"test_staticvec_clear",            test_staticvec_clear},
    {"test_staticvec_iterator",         test_staticvec_iterator},
    {"test_staticvec_copy_push",        test_staticvec_copy_push},
    // List
    {"test_list_push_back_front",       test_list_push_back_front},
    {"test_list_remove",                test_list_remove},
    {"test_list_pop_front_back",        test_list_pop_front_back},
    {"test_list_clear",                 test_list_clear},
    {"test_list_iterator",              test_list_iterator},
    {"test_list_nullptr_ops",           test_list_nullptr_ops},
    // span
    {"test_span_basic",                 test_span_basic},
    {"test_span_default_empty",         test_span_default_empty},
    {"test_span_mutate",                test_span_mutate},
    {"test_span_iterator",              test_span_iterator},
    {"test_span_const",                 test_span_const},
    // array
    {"test_array_size",                 test_array_size},
    {"test_array_access",               test_array_access},
    {"test_array_front_back",           test_array_front_back},
    {"test_array_fill",                 test_array_fill},
    {"test_array_iterator",             test_array_iterator},
    {"test_array_data",                 test_array_data},
    {"test_array_const_access",         test_array_const_access},
};

int run_libfk_stack_queue_staticvec_tests() {
    return run_tests("LibFK Stack/Queue/StaticVector/List/Span/Array",
                     s_tests,
                     sizeof(s_tests) / sizeof(s_tests[0]));
}
