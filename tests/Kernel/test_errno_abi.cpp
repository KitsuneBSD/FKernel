// L1 regression: FKernel exposes the Linux errno ABI to userspace (musl/BusyBox).
// - LibC/errno.h macros must carry the Linux values (not the old BSD ones).
// - fk::core::Error internal values must not collide with Linux errno numbers.
// - error_to_errno() must translate fk errors to the correct Linux errno.

#include <tests/test_framework.h>

#include <LibC/errno.h>
#include <LibFK/Core/error.h>
#include <Kernel/Syscall/syscall_utils.h>

// Linux errno values expected by musl/BusyBox userspace.
static_assert(EAGAIN == 11, "EAGAIN must be Linux 11, not BSD 35");
static_assert(ENOSYS == 38, "ENOSYS must be Linux 38, not BSD 78");
static_assert(ENOTEMPTY == 39, "ENOTEMPTY must be Linux 39, not BSD 66");
static_assert(ENAMETOOLONG == 36, "ENAMETOOLONG must be Linux 36, not BSD 63");
static_assert(ELOOP == 40, "ELOOP must be Linux 40");
static_assert(ETIMEDOUT == 110, "ETIMEDOUT must be Linux 110");
static_assert(EINVAL == 22, "EINVAL must be Linux 22");
static_assert(ENETUNREACH == 101, "ENETUNREACH must be Linux 101");
static_assert(ENETDOWN == 100, "ENETDOWN must be Linux 100");

// fk-specific Error values must not collide with Linux errno numbers.
static_assert(static_cast<int>(fk::core::Error::NotASymlink) != ENETUNREACH,
              "NotASymlink internal value must not collide with ENETUNREACH");
static_assert(static_cast<int>(fk::core::Error::InvalidData) != ENETDOWN,
              "InvalidData internal value must not collide with ENETDOWN");

static const char* test_error_to_errno_translations() {
  TEST_ASSERT_EQ(0, (long)fkernel::error_to_errno(fk::core::Error::None), "None -> 0");
  TEST_ASSERT_EQ(2, (long)fkernel::error_to_errno(fk::core::Error::NotFound), "NotFound -> ENOENT");
  TEST_ASSERT_EQ(11, (long)fkernel::error_to_errno(fk::core::Error::WouldBlock), "WouldBlock -> EAGAIN");
  TEST_ASSERT_EQ(38, (long)fkernel::error_to_errno(fk::core::Error::NotImplemented), "NotImplemented -> ENOSYS");
  TEST_ASSERT_EQ(39, (long)fkernel::error_to_errno(fk::core::Error::DirectoryNotEmpty), "DirectoryNotEmpty -> ENOTEMPTY");
  TEST_ASSERT_EQ(22, (long)fkernel::error_to_errno(fk::core::Error::NotASymlink), "NotASymlink -> EINVAL");
  TEST_ASSERT_EQ(22, (long)fkernel::error_to_errno(fk::core::Error::InvalidData), "InvalidData -> EINVAL");
  TEST_ASSERT_EQ(22, (long)fkernel::error_to_errno(fk::core::Error::InvalidParameter), "InvalidParameter -> EINVAL");
  TEST_ASSERT_EQ(107, (long)fkernel::error_to_errno(fk::core::Error::NotConnected), "NotConnected -> ENOTCONN");
  TEST_ASSERT_EQ(95, (long)fkernel::error_to_errno(fk::core::Error::NotSupported), "NotSupported -> EOPNOTSUPP");
  return nullptr;
}

static const char* test_readlink_non_symlink_is_einval() {
  // readlink(2) on a non-symlink must surface EINVAL to userspace, never ENETUNREACH.
  TEST_ASSERT_EQ(EINVAL, (long)fkernel::error_to_errno(fk::core::Error::NotASymlink),
                 "readlink non-symlink must map to EINVAL");
  return nullptr;
}

static test_case_t s_tests[] = {
  {"error_to_errno_translations", test_error_to_errno_translations},
  {"readlink_non_symlink_is_einval", test_readlink_non_symlink_is_einval},
};

int run_kernel_errno_abi_tests() {
  return run_tests("Kernel::ErrnoABI", s_tests, sizeof(s_tests) / sizeof(s_tests[0]));
}
