// Phase 43b — Dentry cache unit tests (host-side, no kernel boot).
// current_mount_namespace() returns nullptr (see stubs/vfs_stubs.cpp), so
// every Dentry operation uses the default node stack and the built-in
// HashMap child cache.  These tests exercise the metadata path only;
// no Node objects are ever pushed.
#include <tests/test_framework.h>
#include <Kernel/Fs/Vfs/Core/dentry.h>
#include <LibFK/Memory/Pointers/ref_ptr.h>

using fkernel::Dentry;
using fk::RefPtr;

static const char* test_dentry_create_root() {
    auto res = Dentry::create("/");
    TEST_ASSERT(res.is_ok(), "Dentry::create('/') must succeed");
    RefPtr<Dentry> root = res.value();
    TEST_ASSERT(root, "created Dentry must be non-null");
    TEST_ASSERT_STR_EQ("/", root->name().c_str(), "root name must be '/'");
    TEST_ASSERT_STR_EQ("/", root->get_path().c_str(), "root with no parent path must be '/'");
    return nullptr;
}

static const char* test_dentry_create_child_path() {
    auto rr = Dentry::create("/");
    TEST_ASSERT(rr.is_ok(), "root create must succeed");
    RefPtr<Dentry> root = rr.value();

    auto cr = Dentry::create("usr", root);
    TEST_ASSERT(cr.is_ok(), "child create must succeed");
    RefPtr<Dentry> child = cr.value();

    TEST_ASSERT_STR_EQ("usr",  child->name().c_str(),     "child name must be 'usr'");
    TEST_ASSERT_STR_EQ("/usr", child->get_path().c_str(), "child path must be '/usr'");
    return nullptr;
}

static const char* test_dentry_nested_path() {
    auto rr = Dentry::create("/");
    TEST_ASSERT(rr.is_ok(), "root create must succeed");

    auto ar = Dentry::create("a", rr.value());
    TEST_ASSERT(ar.is_ok(), "first-level create must succeed");

    auto br = Dentry::create("b", ar.value());
    TEST_ASSERT(br.is_ok(), "second-level create must succeed");

    TEST_ASSERT_STR_EQ("/a/b", br.value()->get_path().c_str(), "nested path must be '/a/b'");
    return nullptr;
}

static const char* test_dentry_lookup_dot() {
    auto rr = Dentry::create("/");
    TEST_ASSERT(rr.is_ok(), "root create must succeed");
    RefPtr<Dentry> root = rr.value();

    auto res = root->lookup(".");
    TEST_ASSERT(res.is_ok(), "lookup('.') must succeed");
    TEST_ASSERT(res.value().get() == root.get(), "lookup('.') must return self");
    return nullptr;
}

static const char* test_dentry_lookup_dotdot_returns_parent() {
    auto rr = Dentry::create("/");
    TEST_ASSERT(rr.is_ok(), "root create must succeed");
    RefPtr<Dentry> root = rr.value();

    auto cr = Dentry::create("child", root);
    TEST_ASSERT(cr.is_ok(), "child create must succeed");
    RefPtr<Dentry> child = cr.value();

    auto res = child->lookup("..");
    TEST_ASSERT(res.is_ok(), "lookup('..') on child must succeed");
    TEST_ASSERT(res.value().get() == root.get(), "lookup('..') must return parent");
    return nullptr;
}

static const char* test_dentry_lookup_dotdot_root_returns_self() {
    auto rr = Dentry::create("/");
    TEST_ASSERT(rr.is_ok(), "root create must succeed");
    RefPtr<Dentry> root = rr.value();

    auto res = root->lookup("..");
    TEST_ASSERT(res.is_ok(), "root lookup('..') must succeed");
    TEST_ASSERT(res.value().get() == root.get(), "root lookup('..') must return self");
    return nullptr;
}

static const char* test_dentry_add_child_lookup_cached() {
    auto rr = Dentry::create("/");
    TEST_ASSERT(rr.is_ok(), "root create must succeed");
    RefPtr<Dentry> root = rr.value();

    auto cr = Dentry::create("bin", root);
    TEST_ASSERT(cr.is_ok(), "child create must succeed");
    root->add_child(cr.value());

    // Lookup hits the HashMap cache; no node stack traversal needed.
    auto res = root->lookup("bin");
    TEST_ASSERT(res.is_ok(), "lookup of cached child must succeed");
    TEST_ASSERT_STR_EQ("bin", res.value()->name().c_str(),
                        "cached lookup must return the correct child");
    return nullptr;
}

static const char* test_dentry_lookup_missing_returns_not_found() {
    auto rr = Dentry::create("/");
    TEST_ASSERT(rr.is_ok(), "root create must succeed");
    RefPtr<Dentry> root = rr.value();

    // No nodes on the stack and no cached children → must fail.
    auto res = root->lookup("no_such_entry");
    TEST_ASSERT(!res.is_ok(), "lookup of missing entry must fail");
    return nullptr;
}

static const char* test_dentry_multiple_children_paths() {
    auto rr = Dentry::create("/");
    TEST_ASSERT(rr.is_ok(), "root create must succeed");
    RefPtr<Dentry> root = rr.value();

    auto br = Dentry::create("bin", root);
    auto ur = Dentry::create("usr", root);
    TEST_ASSERT(br.is_ok() && ur.is_ok(), "both children must be created");
    root->add_child(br.value());
    root->add_child(ur.value());

    TEST_ASSERT_STR_EQ("/bin", br.value()->get_path().c_str(), "bin path must be '/bin'");
    TEST_ASSERT_STR_EQ("/usr", ur.value()->get_path().c_str(), "usr path must be '/usr'");
    return nullptr;
}

static test_case_t s_tests[] = {
    {"dentry_create_root",                test_dentry_create_root},
    {"dentry_create_child_path",          test_dentry_create_child_path},
    {"dentry_nested_path",                test_dentry_nested_path},
    {"dentry_lookup_dot",                 test_dentry_lookup_dot},
    {"dentry_lookup_dotdot_parent",       test_dentry_lookup_dotdot_returns_parent},
    {"dentry_lookup_dotdot_root_self",    test_dentry_lookup_dotdot_root_returns_self},
    {"dentry_add_child_lookup_cached",    test_dentry_add_child_lookup_cached},
    {"dentry_lookup_missing_not_found",   test_dentry_lookup_missing_returns_not_found},
    {"dentry_multiple_children_paths",    test_dentry_multiple_children_paths},
};

int run_kernel_dentry_tests() {
    return run_tests("Kernel::Dentry",
                     s_tests, sizeof(s_tests) / sizeof(s_tests[0]));
}
