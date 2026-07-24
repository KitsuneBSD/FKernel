#include <Kernel/Arch/x86_64/Interrupt/HardwareInterrupts/tick_manager.h>
#include <Kernel/Arch/x86_64/Syscall/syscall_arch.h>
#include <Kernel/Fs/Vfs/definitions.h>
#include <Kernel/Fs/Vfs/node.h>
#include <Kernel/Scheduler/scheduler.h>
#include <Kernel/Syscall/syscall.h>
#include <LibFK/Core/error.h>
#include <LibFK/Types/types.h>

struct pollfd {
  int fd;
  short events;
  short revents;
};

extern "C" {
uint64_t sys_poll(uint64_t fds_ptr, uint64_t nfds_u64, uint64_t timeout_raw,
                  uint64_t, uint64_t, uint64_t, [[maybe_unused]] PtRegs* regs) {
  int nfds = (int)nfds_u64;
  auto* task = SchedulerManager::the().current();
  if (!task || !fds_ptr || nfds <= 0) return (uint64_t)-1;

  auto* pfds = reinterpret_cast<pollfd*>(fds_ptr);
  int timeout_ms = static_cast<int>(static_cast<int64_t>(timeout_raw));

  bool infinite = (timeout_ms < 0);
  bool non_blocking = (timeout_ms == 0);
  uint64_t deadline = 0;

  if (!infinite && !non_blocking) {
    uint32_t freq = TickManager::the().get_frequency();
    uint64_t ticks = (freq > 0) ? (uint64_t)timeout_ms * freq / 1000 : (uint64_t)timeout_ms;
    deadline = TickManager::the().get_ticks() + (ticks ? ticks : 1);
  }

  while (true) {
    int ready = 0;
    for (int i = 0; i < nfds; ++i) {
      pfds[i].revents = 0;
      int fd = pfds[i].fd;
      if (fd < 0) { pfds[i].revents = POLLNVAL; ready++; continue; }
      if (static_cast<size_t>(fd) >= task->resources.files.descriptors.size()) {
        pfds[i].revents = POLLNVAL; ready++; continue;
      }
      auto& desc = task->resources.files.descriptors[fd];
      if (!desc) { pfds[i].revents = POLLNVAL; ready++; continue; }
      auto node = desc->node();
      if (!node) { pfds[i].revents = POLLHUP; ready++; continue; }

      short cur = node->poll();
      short result = cur & (short)(pfds[i].events | POLLERR | POLLHUP);
      pfds[i].revents = result;
      if (result) ready++;
    }

    if (ready > 0 || non_blocking) return (uint64_t)ready;
    if (!infinite && TickManager::the().get_ticks() >= deadline) return 0;

    SchedulerManager::the().sleep_current(1);
    SchedulerManager::the().schedule();
  }
}
}
