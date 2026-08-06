// Phase 43b — PathResolver unit tests (host-side, no kernel boot).
//
// PathResolver::resolve_unlocked() walks a Dentry tree given a path string.
// SchedulerManager::the().current() returns nullptr in tests (scheduler not
// initialized), so the resolver always falls back to VirtualFileSystem::root().
//
// Test setup:
//   VirtualFileSystem::the().mount_root(mock_node) → creates root Dentry("")
//   Then Dentry::create()/add_child() builds the tree.
//
// Tree used by most tests:
//   / (root)
//   └── usr
//       └── bin
//           └── ls

#include <tests/test_framework.h>
#include <tests/Kernel/mocks/mock_node.h>
#include <Kernel/Fs/Vfs/Core/path_resolver.h>
#include <Kernel/Fs/Vfs/Core/virtual_filesystem.h>
#include <Kernel/Fs/Vfs/Core/dentry.h>
#include <LibFK/Memory/Pointers/ref_ptr.h>
#include <LibFK/Memory/Allocators/new.h>
#include <LibFK/Synchronization/spinlock.h>

using fkernel::Dentry;
using fkernel::PathResolver;
using fkernel::VirtualFileSystem;
using fk::RefPtr;
using fk::make_ref;

// Build the test tree and return a ready PathResolver.
// The VirtualFileSystem singleton is set up once per call.
struct TestTree {
    RefPtr<Dentry> root;
    RefPtr<Dentry> usr;
    RefPtr<Dentry> bin;
    RefPtr<Dentry> ls;
    fk::synchronization::Spinlock lock;
    PathResolver resolver;

    TestTree() : resolver(VirtualFileSystem::the(), lock) {
        // Reset VFS root so each test starts fresh.
        VirtualFileSystem::the().mount_root(make_ref<MockFileNode>().value());
        root = VirtualFileSystem::the().root();

        usr = Dentry::create("usr", root).value();
        root->add_child(usr);

        bin = Dentry::create("bin", usr).value();
        usr->add_child(bin);

        ls = Dentry::create("ls", bin).value();
        bin->add_child(ls);
    }
};

static const char* test_resolve_root() {
    TestTree t;
    auto res = t.resolver.resolve("/");
    TEST_ASSERT(res.is_ok(), "resolve('/') must succeed");
    TEST_ASSERT_STR_EQ("/", res.value()->get_path().c_str(),
                        "resolved dentry path must be '/'");
    return nullptr;
}

static const char* test_resolve_one_level() {
    TestTree t;
    auto res = t.resolver.resolve("/usr");
    TEST_ASSERT(res.is_ok(), "resolve('/usr') must succeed");
    TEST_ASSERT_STR_EQ("/usr", res.value()->get_path().c_str(),
                        "resolved path must be '/usr'");
    return nullptr;
}

static const char* test_resolve_multi_level() {
    TestTree t;
    auto res = t.resolver.resolve("/usr/bin/ls");
    TEST_ASSERT(res.is_ok(), "resolve('/usr/bin/ls') must succeed");
    TEST_ASSERT_STR_EQ("/usr/bin/ls", res.value()->get_path().c_str(),
                        "resolved path must be '/usr/bin/ls'");
    return nullptr;
}

static const char* test_resolve_trailing_slash() {
    TestTree t;
    auto res = t.resolver.resolve("/usr/bin/");
    TEST_ASSERT(res.is_ok(), "trailing slash must be ignored");
    TEST_ASSERT_STR_EQ("/usr/bin", res.value()->get_path().c_str(),
                        "path must be '/usr/bin'");
    return nullptr;
}

static const char* test_resolve_dot_relative() {
    TestTree t;
    // "." relative to usr → same dentry
    auto res = t.resolver.resolve(".", t.usr);
    TEST_ASSERT(res.is_ok(), "resolve('.') must succeed");
    TEST_ASSERT(res.value().get() == t.usr.get(),
                "resolve('.') relative to usr must return usr");
    return nullptr;
}

static const char* test_resolve_dotdot_relative() {
    TestTree t;
    // ".." relative to bin → usr
    auto res = t.resolver.resolve("..", t.bin);
    TEST_ASSERT(res.is_ok(), "resolve('..') must succeed");
    TEST_ASSERT(res.value().get() == t.usr.get(),
                "resolve('..') from bin must return usr");
    return nullptr;
}

static const char* test_resolve_dotdot_at_root_stays_root() {
    TestTree t;
    auto res = t.resolver.resolve("/..");
    TEST_ASSERT(res.is_ok(), "resolve('/..') must succeed");
    TEST_ASSERT(res.value().get() == t.root.get(),
                "'/..' must stay at root");
    return nullptr;
}

static const char* test_resolve_missing_component_fails() {
    TestTree t;
    auto res = t.resolver.resolve("/usr/no_such_dir");
    TEST_ASSERT(res.is_error(), "resolve of missing path must fail");
    return nullptr;
}

static const char* test_resolve_depth_limit() {
    TestTree t;
    // depth > 8 must return IOError
    auto res = t.resolver.resolve("/usr", nullptr, 9);
    TEST_ASSERT(res.is_error(), "depth > 8 must return an error");
    return nullptr;
}

static const char* test_resolve_to_parent_root_child() {
    TestTree t;
    auto res = t.resolver.resolve_to_parent("/usr");
    TEST_ASSERT(res.is_ok(), "resolve_to_parent('/usr') must succeed");
    auto& [parent, name] = res.value();
    TEST_ASSERT(parent.get() == t.root.get(), "parent must be root");
    TEST_ASSERT_STR_EQ("usr", name.c_str(), "filename must be 'usr'");
    return nullptr;
}

static const char* test_resolve_to_parent_deep() {
    TestTree t;
    auto res = t.resolver.resolve_to_parent("/usr/bin/ls");
    TEST_ASSERT(res.is_ok(), "resolve_to_parent('/usr/bin/ls') must succeed");
    auto& [parent, name] = res.value();
    TEST_ASSERT_STR_EQ("/usr/bin", parent->get_path().c_str(),
                        "parent must be '/usr/bin'");
    TEST_ASSERT_STR_EQ("ls", name.c_str(), "filename must be 'ls'");
    return nullptr;
}

static test_case_t s_tests[] = {
    {"resolve_root",                  test_resolve_root},
    {"resolve_one_level",             test_resolve_one_level},
    {"resolve_multi_level",           test_resolve_multi_level},
    {"resolve_trailing_slash",        test_resolve_trailing_slash},
    {"resolve_dot_relative",          test_resolve_dot_relative},
    {"resolve_dotdot_relative",       test_resolve_dotdot_relative},
    {"resolve_dotdot_at_root",        test_resolve_dotdot_at_root_stays_root},
    {"resolve_missing_component",     test_resolve_missing_component_fails},
    {"resolve_depth_limit",           test_resolve_depth_limit},
    {"resolve_to_parent_root_child",  test_resolve_to_parent_root_child},
    {"resolve_to_parent_deep",        test_resolve_to_parent_deep},
};

int run_kernel_path_resolver_tests() {
    return run_tests("Kernel::PathResolver",
                     s_tests, sizeof(s_tests) / sizeof(s_tests[0]));
}
