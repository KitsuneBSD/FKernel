#include <tests/test_framework.h>
#include <Kernel/Memory/PhysicalMemory/Buddy/buddy_state.h>
#include <Kernel/Arch/x86_64/arch_defs.h>

// BuddyState stores FreeBlock nodes inline at (KERNEL_VIRT_BASE + phys).
// On the host, we back the "physical" address space with a real buffer.
// slot_phys(i) = &buf[i] - KERNEL_VIRT_BASE: unsigned wrap ensures
//   KERNEL_VIRT_BASE + slot_phys(i) == &buf[i], giving us valid memory.

static constexpr size_t NUM_SLOTS = 8;
static constexpr size_t SLOT_SIZE = sizeof(FreeBlock);
static constexpr size_t TEST_ORDER = 0; // index 0 → MIN_ORDER = 12 → 4 KiB pages

alignas(alignof(FreeBlock)) static uint8_t g_buf[NUM_SLOTS * SLOT_SIZE];

static uintptr_t slot_phys(size_t i) {
    uintptr_t ptr = reinterpret_cast<uintptr_t>(&g_buf[i * SLOT_SIZE]);
    return ptr - KERNEL_VIRT_BASE; // unsigned wrap: KERNEL_VIRT_BASE + result == ptr
}

static void clear_buf() {
    for (size_t i = 0; i < sizeof(g_buf); ++i) g_buf[i] = 0;
}

static const char* test_buddy_reset_clears_lists() {
    BuddyState bs;
    bs.reset();
    // All orders should be empty after reset — pop returns 0.
    for (size_t i = 0; i < NUM_ORDERS; ++i)
        TEST_ASSERT_EQ(0u, bs.pop(i), "pop on reset list must return 0");
    return nullptr;
}

static const char* test_buddy_push_pop_single() {
    clear_buf();
    BuddyState bs;
    bs.reset();

    uintptr_t phys = slot_phys(0);
    bs.push(TEST_ORDER, phys);

    uintptr_t got = bs.pop(TEST_ORDER);
    TEST_ASSERT_EQ(phys, got, "pop must return the pushed physical address");
    TEST_ASSERT_EQ(0u, bs.pop(TEST_ORDER), "second pop on now-empty list must return 0");
    return nullptr;
}

static const char* test_buddy_push_pop_lifo() {
    clear_buf();
    BuddyState bs;
    bs.reset();

    uintptr_t p0 = slot_phys(0);
    uintptr_t p1 = slot_phys(1);
    uintptr_t p2 = slot_phys(2);

    bs.push(TEST_ORDER, p0);
    bs.push(TEST_ORDER, p1);
    bs.push(TEST_ORDER, p2);

    // push prepends → LIFO order: p2 first, then p1, then p0
    TEST_ASSERT_EQ(p2, bs.pop(TEST_ORDER), "first pop must return last pushed");
    TEST_ASSERT_EQ(p1, bs.pop(TEST_ORDER), "second pop must return second-to-last");
    TEST_ASSERT_EQ(p0, bs.pop(TEST_ORDER), "third pop must return first pushed");
    TEST_ASSERT_EQ(0u, bs.pop(TEST_ORDER), "list empty after three pops");
    return nullptr;
}

static const char* test_buddy_remove_head() {
    clear_buf();
    BuddyState bs;
    bs.reset();

    uintptr_t p0 = slot_phys(0);
    uintptr_t p1 = slot_phys(1);
    bs.push(TEST_ORDER, p0);
    bs.push(TEST_ORDER, p1); // head is p1 (LIFO)

    bool ok = bs.remove(TEST_ORDER, p1); // remove head
    TEST_ASSERT(ok, "remove head must return true");

    uintptr_t got = bs.pop(TEST_ORDER);
    TEST_ASSERT_EQ(p0, got, "only p0 remains after removing head");
    TEST_ASSERT_EQ(0u, bs.pop(TEST_ORDER), "list empty");
    return nullptr;
}

static const char* test_buddy_remove_tail() {
    clear_buf();
    BuddyState bs;
    bs.reset();

    uintptr_t p0 = slot_phys(0);
    uintptr_t p1 = slot_phys(1);
    bs.push(TEST_ORDER, p0); // p0 pushed first → tail after p1 pushed
    bs.push(TEST_ORDER, p1); // head=p1, tail=p0

    bool ok = bs.remove(TEST_ORDER, p0); // remove tail
    TEST_ASSERT(ok, "remove tail must return true");

    uintptr_t got = bs.pop(TEST_ORDER);
    TEST_ASSERT_EQ(p1, got, "only p1 remains after removing tail");
    TEST_ASSERT_EQ(0u, bs.pop(TEST_ORDER), "list empty");
    return nullptr;
}

static const char* test_buddy_remove_middle() {
    clear_buf();
    BuddyState bs;
    bs.reset();

    uintptr_t p0 = slot_phys(0);
    uintptr_t p1 = slot_phys(1);
    uintptr_t p2 = slot_phys(2);
    bs.push(TEST_ORDER, p0);
    bs.push(TEST_ORDER, p1);
    bs.push(TEST_ORDER, p2); // list: p2 → p1 → p0

    bool ok = bs.remove(TEST_ORDER, p1);
    TEST_ASSERT(ok, "remove middle must return true");

    TEST_ASSERT_EQ(p2, bs.pop(TEST_ORDER), "first pop must be p2");
    TEST_ASSERT_EQ(p0, bs.pop(TEST_ORDER), "second pop must be p0");
    TEST_ASSERT_EQ(0u, bs.pop(TEST_ORDER), "list empty after removing middle");
    return nullptr;
}

static const char* test_buddy_remove_unpushed_returns_false() {
    clear_buf(); // zero-initialize: phys_addr fields are 0
    BuddyState bs;
    bs.reset();

    // slot 3 was never pushed — its FreeBlock.phys_addr == 0 (cleared by clear_buf)
    uintptr_t phys = slot_phys(3);
    TEST_ASSERT(phys != 0, "test requires slot_phys != 0 for the guard to fire");
    bool ok = bs.remove(TEST_ORDER, phys);
    TEST_ASSERT(!ok, "remove of unpushed block must return false");
    return nullptr;
}

static const char* test_buddy_different_orders_independent() {
    clear_buf();
    BuddyState bs;
    bs.reset();

    uintptr_t p0 = slot_phys(0);
    uintptr_t p1 = slot_phys(1);

    bs.push(0, p0); // order index 0
    bs.push(1, p1); // order index 1

    TEST_ASSERT_EQ(0u, bs.pop(2), "order 2 must be empty");
    TEST_ASSERT_EQ(p1, bs.pop(1), "order 1 gives p1");
    TEST_ASSERT_EQ(p0, bs.pop(0), "order 0 gives p0");
    return nullptr;
}

static test_case_t s_tests[] = {
    {"buddy_reset_clears_lists",          test_buddy_reset_clears_lists},
    {"buddy_push_pop_single",             test_buddy_push_pop_single},
    {"buddy_push_pop_lifo",               test_buddy_push_pop_lifo},
    {"buddy_remove_head",                 test_buddy_remove_head},
    {"buddy_remove_tail",                 test_buddy_remove_tail},
    {"buddy_remove_middle",               test_buddy_remove_middle},
    {"buddy_remove_unpushed_false",       test_buddy_remove_unpushed_returns_false},
    {"buddy_different_orders_independent",test_buddy_different_orders_independent},
};

int run_kernel_buddy_state_tests() {
    return run_tests("Kernel::BuddyState",
                     s_tests,
                     sizeof(s_tests) / sizeof(s_tests[0]));
}
