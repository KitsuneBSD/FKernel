#include <tests/test_framework.h>
#include <LibFK/Memory/nonnull_own_ptr.h>
#include <LibFK/Memory/nonnull_ref_ptr.h>
#include <LibFK/Memory/ref_counted.h>
#include <LibFK/Memory/weakable.h>
#include <LibFK/Memory/bump_allocator.h>

// ---- helpers ---------------------------------------------------------------

struct Tracked {
    int value;
    static int alive;
    explicit Tracked(int v) : value(v) { ++alive; }
    ~Tracked() { --alive; }
};
int Tracked::alive = 0;

struct RefTracked : public fk::memory::RefCounted<RefTracked> {
    int value;
    static int alive;
    explicit RefTracked(int v) : value(v) { ++alive; }
    ~RefTracked() { --alive; }
};
int RefTracked::alive = 0;

struct WeakTarget : public fk::Weakable<WeakTarget> {
    int value;
    static int alive;
    explicit WeakTarget(int v) : value(v) { ++alive; }
    ~WeakTarget() { --alive; }
};
int WeakTarget::alive = 0;

// ---- NonnullOwnPtr ---------------------------------------------------------

static const char* test_nonnull_own_basic() {
    Tracked::alive = 0;
    {
        auto p = fk::memory::make_nonnull<Tracked>(42);
        TEST_ASSERT_EQ(42, p->value, "NonnullOwnPtr value should be 42");
        TEST_ASSERT_EQ(1, Tracked::alive, "1 alive");
    }
    TEST_ASSERT_EQ(0, Tracked::alive, "0 alive after scope");
    return NULL;
}

static const char* test_nonnull_own_move() {
    Tracked::alive = 0;
    auto a = fk::memory::make_nonnull<Tracked>(7);
    auto b = fk::types::move(a);
    TEST_ASSERT_EQ(7, b->value, "moved value should be 7");
    TEST_ASSERT_EQ(1, Tracked::alive, "still 1 alive after move");
    return NULL;
}

static const char* test_nonnull_own_release() {
    Tracked::alive = 0;
    auto p = fk::memory::make_nonnull<Tracked>(5);
    fk::OwnPtr<Tracked> q = p.release();
    TEST_ASSERT(static_cast<bool>(q), "released OwnPtr should be non-null");
    TEST_ASSERT_EQ(5, q->value, "released value should be 5");
    TEST_ASSERT_EQ(1, Tracked::alive, "still 1 alive after release");
    return NULL;
}

// ---- NonnullRefPtr ---------------------------------------------------------

static const char* test_nonnull_ref_basic() {
    RefTracked::alive = 0;
    {
        fk::NonnullRefPtr<RefTracked> p(new RefTracked(10));
        TEST_ASSERT_EQ(10, p->value, "NonnullRefPtr value should be 10");
        TEST_ASSERT_EQ(1, RefTracked::alive, "1 alive");
        TEST_ASSERT_EQ(1, (int)p->ref_count(), "ref_count should be 1");
    }
    TEST_ASSERT_EQ(0, RefTracked::alive, "0 alive after scope");
    return NULL;
}

static const char* test_nonnull_ref_copy() {
    RefTracked::alive = 0;
    fk::NonnullRefPtr<RefTracked> a(new RefTracked(20));
    {
        fk::NonnullRefPtr<RefTracked> b = a;
        TEST_ASSERT_EQ(2, (int)a->ref_count(), "ref_count 2 with two ptrs");
        TEST_ASSERT_EQ(1, RefTracked::alive, "still 1 object");
    }
    TEST_ASSERT_EQ(1, (int)a->ref_count(), "ref_count back to 1");
    return NULL;
}

static const char* test_nonnull_ref_as_nullable() {
    RefTracked::alive = 0;
    fk::NonnullRefPtr<RefTracked> p(new RefTracked(30));
    auto q = p.as_nullable();
    TEST_ASSERT(static_cast<bool>(q), "nullable should be non-null");
    TEST_ASSERT_EQ(30, q->value, "nullable value should be 30");
    return NULL;
}

// ---- WeakPtr / Weakable ----------------------------------------------------

static const char* test_weakptr_basic() {
    WeakTarget::alive = 0;
    fk::WeakPtr<WeakTarget> weak;
    {
        WeakTarget obj(99);
        weak = obj.make_weak_ptr();
        TEST_ASSERT(!weak.is_null(), "weak should resolve while alive");
        TEST_ASSERT_EQ(99, weak.try_resolve()->value, "resolve should give 99");
    }
    TEST_ASSERT(weak.is_null(), "weak should be null after object destroyed");
    TEST_ASSERT_EQ(0, WeakTarget::alive, "0 alive after scope");
    return NULL;
}

static const char* test_weakptr_copy() {
    WeakTarget::alive = 0;
    fk::WeakPtr<WeakTarget> w1, w2;
    {
        WeakTarget obj(55);
        w1 = obj.make_weak_ptr();
        w2 = w1;
        TEST_ASSERT_EQ(55, w2.try_resolve()->value, "copied weak should resolve");
    }
    TEST_ASSERT(w1.is_null(), "w1 null after object gone");
    TEST_ASSERT(w2.is_null(), "w2 null after object gone");
    return NULL;
}

static const char* test_weakptr_multiple() {
    WeakTarget::alive = 0;
    {
        WeakTarget obj(11);
        auto w1 = obj.make_weak_ptr();
        auto w2 = obj.make_weak_ptr();
        TEST_ASSERT_EQ(11, w1.try_resolve()->value, "w1 resolves");
        TEST_ASSERT_EQ(11, w2.try_resolve()->value, "w2 resolves");
    }
    return NULL;
}

// ---- BumpAllocator ---------------------------------------------------------

static const char* test_bump_basic() {
    fk::BumpAllocator bump(256);
    TEST_ASSERT(bump.is_valid(), "bump should be valid");
    TEST_ASSERT_EQ(0, (int)bump.used(), "used should be 0 initially");

    void* p = bump.allocate(16);
    TEST_ASSERT(p != nullptr, "first allocation should succeed");
    TEST_ASSERT_EQ(16, (int)bump.used(), "used should be 16 after 16-byte alloc");
    return NULL;
}

static const char* test_bump_alignment() {
    fk::BumpAllocator bump(256);
    void* a = bump.allocate(1);   // leaves cursor at offset 1
    void* b = bump.allocate(4, 4); // should align to 4
    TEST_ASSERT(a != nullptr, "first alloc ok");
    TEST_ASSERT(b != nullptr, "aligned alloc ok");
    TEST_ASSERT_EQ(0, (int)((uintptr_t)b % 4), "b should be 4-byte aligned");
    return NULL;
}

static const char* test_bump_overflow() {
    fk::BumpAllocator bump(8);
    void* p = bump.allocate(8);
    TEST_ASSERT(p != nullptr, "first 8 bytes ok");
    void* q = bump.allocate(1);
    TEST_ASSERT(q == nullptr, "overflow should return nullptr");
    return NULL;
}

static const char* test_bump_free_all() {
    fk::BumpAllocator bump(64);
    bump.allocate(32);
    TEST_ASSERT_EQ(32, (int)bump.used(), "32 used after first alloc");
    bump.free_all();
    TEST_ASSERT_EQ(0, (int)bump.used(), "0 used after free_all");
    void* p = bump.allocate(32);
    TEST_ASSERT(p != nullptr, "alloc after reset should succeed");
    return NULL;
}

// ---- registration ----------------------------------------------------------

static const test_case_t nonnull_weak_bump_tests[] = {
    {"nonnull_own_basic",       test_nonnull_own_basic},
    {"nonnull_own_move",        test_nonnull_own_move},
    {"nonnull_own_release",     test_nonnull_own_release},
    {"nonnull_ref_basic",       test_nonnull_ref_basic},
    {"nonnull_ref_copy",        test_nonnull_ref_copy},
    {"nonnull_ref_as_nullable", test_nonnull_ref_as_nullable},
    {"weakptr_basic",           test_weakptr_basic},
    {"weakptr_copy",            test_weakptr_copy},
    {"weakptr_multiple",        test_weakptr_multiple},
    {"bump_basic",              test_bump_basic},
    {"bump_alignment",          test_bump_alignment},
    {"bump_overflow",           test_bump_overflow},
    {"bump_free_all",           test_bump_free_all},
};

int run_libfk_nonnull_weak_bump_tests() {
    return run_tests("LibFK NonnullPtr/WeakPtr/BumpAllocator",
                     nonnull_weak_bump_tests,
                     sizeof(nonnull_weak_bump_tests) / sizeof(nonnull_weak_bump_tests[0]));
}
