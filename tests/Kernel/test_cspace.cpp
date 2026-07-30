#include <tests/test_framework.h>
#include <Kernel/Ipc/cspace.h>

using namespace fkernel::ipc;

// Arbitrary non-null "object" pointers for testing
static int g_obj_a = 1;
static int g_obj_b = 2;
static int g_obj_c = 3;

static const char* test_install_and_get() {
    CSpace cs;
    Capability cap(&g_obj_a, CapabilityType::FileDescription, CapabilityRights::All);
    uint32_t h = cs.install(cap);
    TEST_ASSERT(h != INVALID_HANDLE, "install must return valid handle");
    Capability got = cs.get(h);
    TEST_ASSERT(got.is_valid(), "get must return valid capability");
    TEST_ASSERT(got.object() == &g_obj_a, "get must return same object");
    return nullptr;
}

static const char* test_get_invalid_handle() {
    CSpace cs;
    Capability got = cs.get(999);
    TEST_ASSERT(!got.is_valid(), "get on out-of-range handle must return invalid cap");
    return nullptr;
}

static const char* test_remove() {
    CSpace cs;
    Capability cap(&g_obj_a, CapabilityType::FileDescription, CapabilityRights::All);
    uint32_t h = cs.install(cap);
    cs.remove(h);
    Capability got = cs.get(h);
    TEST_ASSERT(!got.is_valid(), "get after remove must return invalid cap");
    TEST_ASSERT(!cs.contains(h), "contains after remove must be false");
    return nullptr;
}

static const char* test_contains() {
    CSpace cs;
    Capability cap(&g_obj_a, CapabilityType::FileDescription, CapabilityRights::All);
    uint32_t h = cs.install(cap);
    TEST_ASSERT(cs.contains(h), "contains must be true after install");
    cs.remove(h);
    TEST_ASSERT(!cs.contains(h), "contains must be false after remove");
    return nullptr;
}

static const char* test_find_by_object() {
    CSpace cs;
    Capability cap(&g_obj_b, CapabilityType::FileDescription, CapabilityRights::Read);
    cs.install(cap);
    Capability found = cs.find_by_object(&g_obj_b);
    TEST_ASSERT(found.is_valid(), "find_by_object must find installed cap");
    TEST_ASSERT(found.object() == &g_obj_b, "find_by_object must return correct object");
    return nullptr;
}

static const char* test_find_by_object_missing() {
    CSpace cs;
    Capability found = cs.find_by_object(&g_obj_a);
    TEST_ASSERT(!found.is_valid(), "find_by_object on absent object must return invalid cap");
    return nullptr;
}

static const char* test_remove_by_object() {
    CSpace cs;
    Capability cap(&g_obj_c, CapabilityType::FileDescription, CapabilityRights::All);
    cs.install(cap);
    cs.remove_by_object(&g_obj_c);
    Capability found = cs.find_by_object(&g_obj_c);
    TEST_ASSERT(!found.is_valid(), "find_by_object after remove_by_object must return invalid");
    return nullptr;
}

static const char* test_grant() {
    CSpace src;
    CSpace dst;
    Capability cap(&g_obj_a, CapabilityType::FileDescription, CapabilityRights::All);
    uint32_t src_h = src.install(cap);
    uint32_t dst_h = src.grant(dst, src_h, CapabilityRights::Read);
    TEST_ASSERT(dst_h != INVALID_HANDLE, "grant must return valid handle in dest");
    TEST_ASSERT(src.contains(src_h), "grant must not remove from source");
    Capability got = dst.get(dst_h);
    TEST_ASSERT(got.is_valid(), "granted cap must be valid in dest");
    return nullptr;
}

static const char* test_transfer() {
    CSpace src;
    CSpace dst;
    Capability cap(&g_obj_b, CapabilityType::FileDescription, CapabilityRights::All);
    uint32_t src_h = src.install(cap);
    uint32_t dst_h = src.transfer(dst, src_h);
    TEST_ASSERT(dst_h != INVALID_HANDLE, "transfer must return valid handle in dest");
    TEST_ASSERT(!src.contains(src_h), "transfer must remove from source");
    Capability got = dst.get(dst_h);
    TEST_ASSERT(got.is_valid(), "transferred cap must be valid in dest");
    return nullptr;
}

static const char* test_grant_all_to() {
    CSpace src;
    CSpace dst;
    Capability cap_a(&g_obj_a, CapabilityType::FileDescription, CapabilityRights::Read);
    Capability cap_b(&g_obj_b, CapabilityType::Endpoint,         CapabilityRights::Send);
    src.install(cap_a);
    src.install(cap_b);
    src.grant_all_to(dst, CapabilityType::FileDescription);
    Capability found = dst.find_by_object(&g_obj_a);
    TEST_ASSERT(found.is_valid(), "grant_all_to must copy FileDescription cap");
    Capability notfound = dst.find_by_object(&g_obj_b);
    TEST_ASSERT(!notfound.is_valid(), "grant_all_to with type filter must skip Endpoint cap");
    return nullptr;
}

static const char* test_size() {
    CSpace cs;
    TEST_ASSERT_EQ(0, (long)cs.size(), "empty CSpace must have size 0");
    Capability cap(&g_obj_a, CapabilityType::FileDescription, CapabilityRights::All);
    uint32_t h1 = cs.install(cap);
    TEST_ASSERT_EQ(1, (long)cs.size(), "size must be 1 after one install");
    cs.install(cap);
    TEST_ASSERT_EQ(2, (long)cs.size(), "size must be 2 after two installs");
    cs.remove(h1);
    TEST_ASSERT_EQ(1, (long)cs.size(), "size must be 1 after remove");
    return nullptr;
}

static const char* test_free_list_reuse() {
    CSpace cs;
    Capability cap(&g_obj_a, CapabilityType::FileDescription, CapabilityRights::All);
    uint32_t h1 = cs.install(cap);
    cs.remove(h1);
    Capability cap2(&g_obj_b, CapabilityType::FileDescription, CapabilityRights::All);
    uint32_t h2 = cs.install(cap2);
    // Slot must be reused (free-list)
    TEST_ASSERT_EQ((long)h1, (long)h2, "free-list must reuse removed slot");
    Capability got = cs.get(h2);
    TEST_ASSERT(got.object() == &g_obj_b, "reused slot must hold new capability");
    return nullptr;
}

int run_kernel_cspace_tests() {
    static const test_case_t tests[] = {
        {"install_and_get",        test_install_and_get},
        {"get_invalid_handle",     test_get_invalid_handle},
        {"remove",                 test_remove},
        {"contains",               test_contains},
        {"find_by_object",         test_find_by_object},
        {"find_by_object_missing", test_find_by_object_missing},
        {"remove_by_object",       test_remove_by_object},
        {"grant",                  test_grant},
        {"transfer",               test_transfer},
        {"grant_all_to",           test_grant_all_to},
        {"size",                   test_size},
        {"free_list_reuse",        test_free_list_reuse},
    };
    return run_tests("Kernel::CSpace", tests,
                     sizeof(tests) / sizeof(tests[0]));
}
