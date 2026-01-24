#include <Kernel/Fs/Vfs/virtual_filesystem.h>
#include <Kernel/Scheduler/scheduler.h>
#include <Kernel/Syscall/syscall_utils.h>
#include <LibC/string.h>

struct winsize {
    unsigned short ws_row;
    unsigned short ws_col;
    unsigned short ws_xpixel;
    unsigned short ws_ypixel;
};

#define TIOCGWINSZ 0x5413

extern "C" {

uint64_t sys_ioctl(uint64_t fd, uint64_t request, uint64_t arg, uint64_t, uint64_t, uint64_t) {
  auto *task = SchedulerManager::the().current();
  if (!task) return -1;

  auto description = task->get_file_descriptor(static_cast<int>(fd));
  if (!description) return -1;

  auto res = description->ioctl(request, arg);
  if (res.is_error()) return fkernel::return_error(res.error());

  return res.value();
}
}
