#include <Kernel/Arch/x86_64/Syscall/syscall_arch.h>
#include <Kernel/Clock/clock_interrupt.h>
#include <Kernel/Fs/Vfs/Core/dentry.h>
#include <Kernel/Fs/Vfs/Core/timespec.h>
#include <Kernel/Fs/Vfs/Core/virtual_filesystem.h>
#include <Kernel/Memory/UserAccess/user_access.h>
#include <Kernel/Scheduler/Core/scheduler.h>
#include <Kernel/Syscall/syscall_utils.h>
#include <LibFK/Utilities/memory.h>

using namespace fkernel;

static constexpr int AT_FDCWD = -100;
static constexpr int64_t UTIME_NOW = ((1l << 30) - 1);
static constexpr int64_t UTIME_OMIT = ((1l << 30) - 2);

static int64_t current_epoch_seconds() {
  auto dt = ClockManager::the().datetime();
  uint64_t y = dt.year - 1970;
  uint64_t days = y * 365 + (y + 2) / 4;
  static const int days_in_month[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
  for (int i = 0; i < dt.month - 1 && i < 12; ++i)
    days += days_in_month[i];
  if (dt.month > 2 && (dt.year % 4 == 0))
    days++;
  days += (dt.day - 1);
  return static_cast<int64_t>(days * 86400 + dt.hour * 3600 + dt.minute * 60 + dt.second);
}

// sys_utimensat(dirfd, path, times[2], flags) → 0 or -errno
// times == NULL means set both to UTIME_NOW. Per-entry tv_nsec may be
// UTIME_NOW (-1 convention in glibc is (1<<30)-1) or UTIME_OMIT.
extern "C" uint64_t sys_utimensat(uint64_t dirfd_u, uint64_t path_ptr, uint64_t times_ptr,
                       uint64_t flags_u, uint64_t, uint64_t, [[maybe_unused]] PtRegs* regs) {
  auto* task = SchedulerManager::the().current();
  if (!task)
    return (uint64_t)-1;
  const char* path = reinterpret_cast<const char*>(path_ptr);
  if (!path)
    return return_error(fk::core::Error::InvalidParameter);
  (void)flags_u;
  (void)AT_FDCWD;
  (void)dirfd_u;

  timespec user_times[2];
  bool have_times = false;
  if (times_ptr != 0) {
    auto copy_res = memory::copy_from_user(user_times,
                                           reinterpret_cast<const void*>(times_ptr),
                                           sizeof(user_times));
    if (copy_res.is_error())
      return return_error(copy_res.error());
    for (int i = 0; i < 2; ++i) {
      if (user_times[i].tv_nsec != UTIME_NOW && user_times[i].tv_nsec != UTIME_OMIT &&
          !(user_times[i].tv_nsec >= 0 && user_times[i].tv_nsec < 1000000000LL))
        return return_error(fk::core::Error::InvalidParameter);
    }
    have_times = true;
  }

  char resolved[512];
  if (path[0] == '/') {
    fk::memory::copy_n(resolved, path, sizeof(resolved) - 1);
    resolved[sizeof(resolved) - 1] = '\0';
  } else {
    // AT_FDCWD or any real dirfd: best-effort resolve against task cwd.
    const char* cwd = task->resources.files.cwd.c_str();
    size_t cwd_len = fk::memory::length(cwd);
    if (cwd_len + fk::memory::length(path) + 2 >= sizeof(resolved))
      return return_error(fk::core::Error::InvalidParameter);
    fk::memory::copy_string(resolved, cwd);
    if (cwd_len > 0 && resolved[cwd_len - 1] != '/') {
      resolved[cwd_len++] = '/';
      resolved[cwd_len] = '\0';
    }
    fk::memory::concatenate(resolved, path);
  }

  auto res = VirtualFileSystem::the().resolve_path(resolved);
  if (res.is_error())
    return return_error(res.error());
  auto node = res.value()->top_node();
  if (!node)
    return return_error(fk::core::Error::NotFound);

  timespec atime{0, 0};
  timespec mtime{0, 0};
  if (have_times) {
    atime = user_times[0];
    mtime = user_times[1];
  } else {
    atime.tv_nsec = UTIME_NOW;
    mtime.tv_nsec = UTIME_NOW;
  }

  int64_t now = current_epoch_seconds();
  if (atime.tv_nsec == UTIME_NOW) {
    atime.tv_sec = now;
    atime.tv_nsec = 0;
  } else if (atime.tv_nsec == UTIME_OMIT) {
    atime = node->atime();
  }
  if (mtime.tv_nsec == UTIME_NOW) {
    mtime.tv_sec = now;
    mtime.tv_nsec = 0;
  } else if (mtime.tv_nsec == UTIME_OMIT) {
    mtime = node->mtime();
  }

  auto apply_res = node->set_times(atime, mtime);
  if (apply_res.is_error())
    return return_error(apply_res.error());
  return 0;
}
