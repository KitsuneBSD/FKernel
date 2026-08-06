#include <Kernel/Arch/x86_64/Interrupt/HardwareInterrupts/tick_manager.h>
#include <Kernel/Arch/x86_64/Syscall/syscall_arch.h>
#include <Kernel/Ipc/Notifications/notification.h>
#include <Kernel/Memory/UserAccess/user_access.h>
#include <Kernel/Scheduler/Core/scheduler.h>
#include <Kernel/Syscall/syscall.h>
#include <Kernel/Syscall/syscall_utils.h>

static constexpr size_t FUTEX_HASH_SIZE = 256;
static fkernel::ipc::Notification s_futex_notifications[FUTEX_HASH_SIZE];

static size_t futex_bucket(uintptr_t uaddr) {
  return (uaddr >> 2) & (FUTEX_HASH_SIZE - 1);
}

extern "C" void fkernel_futex_wake_one(uintptr_t uaddr) {
  s_futex_notifications[futex_bucket(uaddr)].signal(fk::NotificationBits(1));
}

static constexpr int FUTEX_WAIT         = 0;
static constexpr int FUTEX_WAKE         = 1;
static constexpr int FUTEX_REQUEUE      = 3;
static constexpr int FUTEX_CMP_REQUEUE  = 4;
static constexpr int FUTEX_PRIVATE_FLAG = 128;

extern "C" uint64_t sys_futex(uint64_t uaddr, uint64_t op, uint64_t val,
                               uint64_t timeout_ptr, uint64_t uaddr2, uint64_t val3,
                               [[maybe_unused]] PtRegs* regs) {
  (void)uaddr2; (void)val3;
  int cmd = (int)(op & ~FUTEX_PRIVATE_FLAG);
  uintptr_t addr = static_cast<uintptr_t>(uaddr);

  switch (cmd) {
  case FUTEX_WAIT: {
    if (!fkernel::memory::is_user_address(addr, sizeof(int)))
      return (uint64_t)-14;

    auto& notif = s_futex_notifications[futex_bucket(addr)];

    int cur_val = 0;
    auto res = fkernel::memory::copy_from_user(&cur_val,
                                                reinterpret_cast<const void*>(addr),
                                                sizeof(int));
    if (res.is_error()) return (uint64_t)-14;
    if (cur_val != (int)val) return (uint64_t)-11;

    if (timeout_ptr) {
      struct { int64_t tv_sec; int64_t tv_nsec; } ts{};
      auto ts_res = fkernel::memory::copy_from_user(&ts,
                                                     reinterpret_cast<const void*>(timeout_ptr),
                                                     sizeof(ts));
      if (ts_res.is_error()) return (uint64_t)-14;

      uint32_t freq = TickManager::the().get_frequency();
      if (freq == 0) freq = 1000;
      uint64_t ns = (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
      uint64_t ticks = (ns / 1000000ULL) * freq / 1000ULL;
      if (ticks == 0) ticks = 1;

      fk::NotificationBits result = notif.wait_timeout(fk::TickCount(ticks));
      if (result.is_empty())
        return (uint64_t)-110;
    } else {
      notif.wait();
    }

    return 0;
  }

  case FUTEX_WAKE: {
    uint32_t count = (uint32_t)val;
    if (count == 0) count = 1;
    auto& notif = s_futex_notifications[futex_bucket(addr)];
    for (uint32_t i = 0; i < count; ++i)
      notif.signal(fk::NotificationBits(1));
    return (uint64_t)count;
  }

  case FUTEX_REQUEUE:
  case FUTEX_CMP_REQUEUE: {
    if (cmd == FUTEX_CMP_REQUEUE) {
      int cur_val = 0;
      auto chk = fkernel::memory::copy_from_user(&cur_val,
                                                  reinterpret_cast<const void*>(addr),
                                                  sizeof(int));
      if (chk.is_error()) return (uint64_t)-14;
      if (cur_val != (int)val3) return (uint64_t)-11;
    }

    uint32_t nr_wake = (uint32_t)val;
    if (nr_wake == 0) nr_wake = 1;

    auto& src_notif = s_futex_notifications[futex_bucket(addr)];
    for (uint32_t i = 0; i < nr_wake; ++i)
      src_notif.signal(fk::NotificationBits(1));

    return (uint64_t)nr_wake;
  }

  default:
    return (uint64_t)-22;
  }
}
