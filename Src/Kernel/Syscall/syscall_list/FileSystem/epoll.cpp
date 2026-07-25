#include <Kernel/Arch/x86_64/Interrupt/HardwareInterrupts/tick_manager.h>
#include <Kernel/Arch/x86_64/Syscall/syscall_arch.h>
#include <Kernel/Fs/Epoll/epoll_node.h>
#include <Kernel/Fs/Vfs/dentry.h>
#include <Kernel/Fs/Vfs/node.h>
#include <Kernel/Memory/UserAccess/user_access.h>
#include <Kernel/Scheduler/scheduler.h>
#include <Kernel/Syscall/syscall.h>
#include <Kernel/Syscall/syscall_utils.h>

static constexpr int EPOLL_CTL_ADD = 1;
static constexpr int EPOLL_CTL_DEL = 2;
static constexpr int EPOLL_CTL_MOD = 3;

static constexpr uint32_t EPOLLIN  = 0x001;
static constexpr uint32_t EPOLLOUT = 0x004;
static constexpr uint32_t EPOLLERR = 0x008;
static constexpr uint32_t EPOLLHUP = 0x010;

struct epoll_data_t { uint64_t u64; };
struct epoll_event { uint32_t events; epoll_data_t data; };

static int check_epoll_ready(Task* task, EpollNode* epoll,
                              epoll_event* out, int maxevents) {
  int ready = 0;
  for (size_t i = 0; i < epoll->entries().size() && ready < maxevents; ++i) {
    const auto& e = epoll->entries()[i];
    auto desc = task->get_file_descriptor(e.fd);
    if (!desc) continue;
    auto node = desc->node();
    if (!node) continue;
    short result = node->poll() & (short)(e.events | EPOLLERR | EPOLLHUP);
    if (!result) continue;
    out[ready].events      = (uint32_t)result;
    out[ready].data.u64    = e.data_u64;
    ++ready;
  }
  return ready;
}

extern "C" {

uint64_t sys_epoll_create1(uint64_t flags, uint64_t, uint64_t, uint64_t,
                            uint64_t, uint64_t, [[maybe_unused]] PtRegs* regs) {
  (void)flags;
  auto* task = SchedulerManager::the().current();
  if (!task) return (uint64_t)-1;

  auto node_res = fk::make_ref<EpollNode>();
  if (node_res.is_error()) return (uint64_t)-12;

  auto dentry_res = fkernel::Dentry::create("epoll", nullptr);
  if (dentry_res.is_error()) return (uint64_t)-12;

  auto dentry = dentry_res.value();
  dentry->push_node(node_res.value());

  auto desc_res = fk::make_ref<FileDescription>(dentry, 0);
  if (desc_res.is_error()) return (uint64_t)-12;

  return (uint64_t)task->add_file_descriptor(desc_res.value());
}

uint64_t sys_epoll_ctl(uint64_t epfd, uint64_t op, uint64_t fd, uint64_t event_ptr,
                        uint64_t, uint64_t, [[maybe_unused]] PtRegs* regs) {
  auto* task = SchedulerManager::the().current();
  if (!task) return (uint64_t)-1;

  auto epoll_desc = task->get_file_descriptor((int)epfd);
  if (!epoll_desc) return fkernel::return_error(fk::core::Error::InvalidHandle);

  auto node = epoll_desc->node();
  if (!node) return (uint64_t)-9;

  auto* epoll = static_cast<EpollNode*>(node.get());
  epoll_event kev{};
  uint32_t events = EPOLLIN | EPOLLOUT;
  uint64_t data_u64 = 0;
  if (event_ptr && fkernel::memory::is_user_address(event_ptr, sizeof(epoll_event))) {
    auto res = fkernel::memory::copy_from_user(&kev, reinterpret_cast<const void*>(event_ptr), sizeof(kev));
    if (res.is_ok()) { events = kev.events; data_u64 = kev.data.u64; }
  }

  if (op == EPOLL_CTL_ADD) epoll->ctl_add((int)fd, events, data_u64);
  if (op == EPOLL_CTL_DEL) epoll->ctl_del((int)fd);
  if (op == EPOLL_CTL_MOD) epoll->ctl_mod((int)fd, events, data_u64);
  return 0;
}

uint64_t sys_epoll_wait(uint64_t epfd, uint64_t events_ptr, uint64_t maxevents,
                         uint64_t timeout, uint64_t, uint64_t,
                         [[maybe_unused]] PtRegs* regs) {
  auto* task = SchedulerManager::the().current();
  if (!task) return (uint64_t)-1;

  auto epoll_desc = task->get_file_descriptor((int)epfd);
  if (!epoll_desc) return fkernel::return_error(fk::core::Error::InvalidHandle);

  auto node = epoll_desc->node();
  if (!node) return (uint64_t)-9;

  auto* epoll     = static_cast<EpollNode*>(node.get());
  int   max       = (int)maxevents;
  if (max <= 0 || max > 128) max = 128;
  if (!fkernel::memory::is_user_address(events_ptr, (size_t)max * sizeof(epoll_event)))
    return (uint64_t)-14; // EFAULT

  int64_t timeout_ms  = (int64_t)(int32_t)timeout;
  bool  infinite  = (timeout_ms < 0);
  bool  no_wait   = (timeout_ms == 0);

  uint32_t freq     = TickManager::the().get_frequency();
  uint64_t deadline = 0;
  if (!infinite && !no_wait && freq > 0)
    deadline = TickManager::the().get_ticks() + (uint64_t)timeout_ms * freq / 1000;

  epoll_event kout[128];
  while (true) {
    int ready = check_epoll_ready(task, epoll, kout, max);
    if (ready > 0 || no_wait) {
      if (ready > 0)
        fkernel::memory::copy_to_user(reinterpret_cast<void*>(events_ptr), kout,
                                      (size_t)ready * sizeof(epoll_event));
      return (uint64_t)ready;
    }
    if (!infinite && TickManager::the().get_ticks() >= deadline) return 0;
    SchedulerManager::the().sleep_current(1);
    SchedulerManager::the().schedule();
  }
}

}
