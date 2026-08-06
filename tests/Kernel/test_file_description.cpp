// Phase 43b — FileDescription unit tests (host-side, no kernel boot).
//
// FileDescription wraps a Dentry + a file offset.  These tests verify offset
// tracking, seek semantics, permission flag enforcement, and cloexec storage.
// A MockFileNode (64 bytes of sequential data) is pushed on a Dentry to make
// resolve_dentry() succeed.

#include <tests/test_framework.h>
#include <tests/Kernel/mocks/mock_node.h>
#include <Kernel/Fs/Vfs/Core/file_description.h>
#include <Kernel/Fs/Vfs/Core/dentry.h>
#include <Kernel/Fs/Vfs/Core/definitions.h>
#include <LibFK/Memory/Pointers/ref_ptr.h>
#include <LibFK/Memory/Allocators/new.h>

using fkernel::Dentry;
using fk::RefPtr;
using fk::make_ref;

// Build a Dentry backed by a MockFileNode.
static fk::RefPtr<FileDescription> make_fd(int flags = O_RDWR) {
    auto root = Dentry::create("/").value();
    auto child = Dentry::create("f", root).value();
    auto node = make_ref<MockFileNode>(64).value();
    child->push_node(node);
    return make_ref<FileDescription>(child, flags).value();
}

static const char* test_fd_initial_offset_is_zero() {
    auto fd = make_fd();
    TEST_ASSERT_EQ(0, (long)fd->offset(), "initial offset must be 0");
    return nullptr;
}

static const char* test_fd_read_advances_offset() {
    auto fd = make_fd(O_RDONLY);
    uint8_t buf[16];
    auto res = fd->read(16, buf);
    TEST_ASSERT(res.is_ok(), "read must succeed");
    TEST_ASSERT_EQ(16, (long)res.value(), "read must return 16 bytes");
    TEST_ASSERT_EQ(16, (long)fd->offset(), "offset must advance to 16 after read");
    return nullptr;
}

static const char* test_fd_write_advances_offset() {
    auto fd = make_fd(O_WRONLY);
    const uint8_t buf[8] = {0xAA, 0xBB, 0, 0, 0, 0, 0, 0};
    auto res = fd->write(8, buf);
    TEST_ASSERT(res.is_ok(), "write must succeed");
    TEST_ASSERT_EQ(8, (long)res.value(), "write must return 8");
    TEST_ASSERT_EQ(8, (long)fd->offset(), "offset must advance to 8 after write");
    return nullptr;
}

static const char* test_fd_seek_set() {
    auto fd = make_fd();
    auto res = fd->seek(32, SeekMode::Set);
    TEST_ASSERT(res.is_ok(), "seek Set must succeed");
    TEST_ASSERT_EQ(32, (long)res.value(), "seek Set must return new offset");
    TEST_ASSERT_EQ(32, (long)fd->offset(), "offset must be 32");
    return nullptr;
}

static const char* test_fd_seek_cur() {
    auto fd = make_fd();
    fd->seek(10, SeekMode::Set);
    auto res = fd->seek(5, SeekMode::Current);
    TEST_ASSERT(res.is_ok(), "seek Current must succeed");
    TEST_ASSERT_EQ(15, (long)res.value(), "seek Current must return 15");
    return nullptr;
}

static const char* test_fd_seek_end() {
    auto fd = make_fd();  // node has 64 bytes
    auto res = fd->seek(0, SeekMode::End);
    TEST_ASSERT(res.is_ok(), "seek End must succeed");
    TEST_ASSERT_EQ(64, (long)res.value(), "seek End+0 must return file size 64");
    return nullptr;
}

static const char* test_fd_flags_stored() {
    auto fd = make_fd(O_RDONLY);
    TEST_ASSERT_EQ(O_RDONLY, (long)fd->open_flags(), "flags must be stored");
    fd->set_open_flags(O_RDWR);
    TEST_ASSERT_EQ(O_RDWR, (long)fd->open_flags(), "set_open_flags must update flags");
    return nullptr;
}

static const char* test_fd_cloexec_get_set() {
    auto fd = make_fd();
    TEST_ASSERT(!fd->is_cloexec(), "cloexec must start false");
    fd->set_cloexec(true);
    TEST_ASSERT(fd->is_cloexec(), "set_cloexec(true) must be reflected");
    fd->set_cloexec(false);
    TEST_ASSERT(!fd->is_cloexec(), "set_cloexec(false) must clear it");
    return nullptr;
}

static const char* test_fd_write_denied_on_rdonly() {
    auto fd = make_fd(O_RDONLY);
    const uint8_t buf[4] = {};
    auto res = fd->write(4, buf);
    TEST_ASSERT(res.is_error(), "write on O_RDONLY must fail");
    TEST_ASSERT(res.error() == fk::core::Error::PermissionDenied,
                "error must be PermissionDenied");
    return nullptr;
}

static const char* test_fd_read_denied_on_wronly() {
    auto fd = make_fd(O_WRONLY);
    uint8_t buf[4];
    auto res = fd->read(4, buf);
    TEST_ASSERT(res.is_error(), "read on O_WRONLY must fail");
    TEST_ASSERT(res.error() == fk::core::Error::PermissionDenied,
                "error must be PermissionDenied");
    return nullptr;
}

static test_case_t s_tests[] = {
    {"fd_initial_offset_is_zero",    test_fd_initial_offset_is_zero},
    {"fd_read_advances_offset",      test_fd_read_advances_offset},
    {"fd_write_advances_offset",     test_fd_write_advances_offset},
    {"fd_seek_set",                  test_fd_seek_set},
    {"fd_seek_cur",                  test_fd_seek_cur},
    {"fd_seek_end",                  test_fd_seek_end},
    {"fd_flags_stored",              test_fd_flags_stored},
    {"fd_cloexec_get_set",           test_fd_cloexec_get_set},
    {"fd_write_denied_on_rdonly",    test_fd_write_denied_on_rdonly},
    {"fd_read_denied_on_wronly",     test_fd_read_denied_on_wronly},
};

int run_kernel_file_description_tests() {
    return run_tests("Kernel::FileDescription",
                     s_tests, sizeof(s_tests) / sizeof(s_tests[0]));
}
