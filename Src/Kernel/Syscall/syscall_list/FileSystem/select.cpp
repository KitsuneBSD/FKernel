#include <Kernel/Arch/x86_64/Interrupt/HardwareInterrupts/tick_manager.h>
#include <Kernel/Arch/x86_64/Syscall/syscall_arch.h>
#include <Kernel/Fs/Vfs/definitions.h>
#include <Kernel/Fs/Vfs/node.h>
#include <Kernel/Memory/UserAccess/user_access.h>
#include <Kernel/Scheduler/scheduler.h>
#include <Kernel/Syscall/syscall.h>
#include <LibFK/Core/error.h>
#include <LibFK/Types/types.h>

constexpr int FD_SETSIZE = 1024;

struct fd_set {
  unsigned long fds_bits[FD_SETSIZE / (8 * sizeof(unsigned long))];
};

struct timeval {
  long tv_sec;
  long tv_usec;
};

#define FD_ZERO(s) __builtin_memset((s), 0, sizeof(fd_set))
#define FD_SET(fd, s) \
  ((s)->fds_bits[(fd) / (8 * sizeof(unsigned long))] |= (1UL << ((fd) % (8 * sizeof(unsigned long)))))
#define FD_ISSET(fd, s) \
  (((s)->fds_bits[(fd) / (8 * sizeof(unsigned long))] & (1UL << ((fd) % (8 * sizeof(unsigned long))))) != 0)

static int check_ready(Task* task, int nfds, const fd_set* in_read, const fd_set* in_write,
                        fd_set* out_read, fd_set* out_write) {
  int ready = 0;
  if (out_read) FD_ZERO(out_read);
  if (out_write) FD_ZERO(out_write);

  for (int fd = 0; fd < nfds; ++fd) {
    bool want_r = in_read && FD_ISSET(fd, in_read);
    bool want_w = in_write && FD_ISSET(fd, in_write);
    if (!want_r && !want_w) continue;
    if (static_cast<size_t>(fd) >= task->resources.files.descriptors.size()) continue;

    auto& desc = task->resources.files.descriptors[fd];
    if (!desc) continue;
    auto node = desc->node();
    if (!node) continue;

    short ev = node->poll();
    if (want_r && (ev & POLLIN)) {
      if (out_read) FD_SET(fd, out_read);
      ready++;
    }
    if (want_w && (ev & POLLOUT)) {
      if (out_write) FD_SET(fd, out_write);
      ready++;
    }
  }
  return ready;
}

extern "C" {
uint64_t sys_select(uint64_t nfds_u64, uint64_t readfds_ptr, uint64_t writefds_ptr,
                    uint64_t exceptfds_ptr, uint64_t timeout_ptr, uint64_t,
                    [[maybe_unused]] PtRegs* regs) {
  int nfds = (int)nfds_u64;
  auto* task = SchedulerManager::the().current();
  if (!task) return (uint64_t)-1;
  if (nfds <= 0) return 0;
  if (nfds > FD_SETSIZE) nfds = FD_SETSIZE;

  // Copy fd_sets from user space into kernel buffers
  fd_set k_readfds{}, k_writefds{};
  bool has_read = false, has_write = false, has_except = false;

  if (readfds_ptr && fkernel::memory::is_user_address(readfds_ptr, sizeof(fd_set))) {
    fkernel::memory::copy_from_user(&k_readfds, reinterpret_cast<const void*>(readfds_ptr), sizeof(fd_set));
    has_read = true;
  }
  if (writefds_ptr && fkernel::memory::is_user_address(writefds_ptr, sizeof(fd_set))) {
    fkernel::memory::copy_from_user(&k_writefds, reinterpret_cast<const void*>(writefds_ptr), sizeof(fd_set));
    has_write = true;
  }
  if (exceptfds_ptr && fkernel::memory::is_user_address(exceptfds_ptr, sizeof(fd_set)))
    has_except = true;

  fd_set orig_read = k_readfds, orig_write = k_writefds;

  bool infinite = (timeout_ptr == 0);
  bool non_blocking = false;
  uint64_t deadline = 0;

  if (!infinite && fkernel::memory::is_user_address(timeout_ptr, sizeof(timeval))) {
    timeval tv{};
    fkernel::memory::copy_from_user(&tv, reinterpret_cast<const void*>(timeout_ptr), sizeof(tv));
    uint64_t ms = (uint64_t)tv.tv_sec * 1000 + (uint64_t)tv.tv_usec / 1000;
    non_blocking = (tv.tv_sec == 0 && tv.tv_usec == 0);
    if (!non_blocking) {
      uint32_t freq = TickManager::the().get_frequency();
      uint64_t ticks = (freq > 0) ? ms * freq / 1000 : ms;
      deadline = TickManager::the().get_ticks() + (ticks ? ticks : 1);
    }
  }

  while (true) {
    fd_set out_read = orig_read, out_write = orig_write;
    int ready = check_ready(task, nfds,
                             has_read ? &orig_read : nullptr,
                             has_write ? &orig_write : nullptr,
                             has_read ? &out_read : nullptr,
                             has_write ? &out_write : nullptr);

    if (ready > 0 || non_blocking) {
      if (has_read)
        fkernel::memory::copy_to_user(reinterpret_cast<void*>(readfds_ptr), &out_read, sizeof(fd_set));
      if (has_write)
        fkernel::memory::copy_to_user(reinterpret_cast<void*>(writefds_ptr), &out_write, sizeof(fd_set));
      if (has_except) {
        fd_set z{};
        fkernel::memory::copy_to_user(reinterpret_cast<void*>(exceptfds_ptr), &z, sizeof(fd_set));
      }
      return (uint64_t)ready;
    }
    if (!infinite && TickManager::the().get_ticks() >= deadline) return 0;

    SchedulerManager::the().sleep_current(1);
    SchedulerManager::the().schedule();
  }
}
}
