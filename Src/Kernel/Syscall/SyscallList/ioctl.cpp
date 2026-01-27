#include <Kernel/Arch/x86_64/Syscall/syscall_arch.h>
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

#define TCGETS 0x5401
#define TCSETS 0x5402
#define TIOCGWINSZ 0x5413

extern "C" {

uint64_t sys_ioctl(uint64_t fd, uint64_t request, uint64_t arg, uint64_t, uint64_t, uint64_t, [[maybe_unused]] PtRegs* regs) {
  auto *task = SchedulerManager::the().current();
  if (!task) return fkernel::return_error(fk::core::Error::PermissionDenied);

  auto description = task->get_file_descriptor(static_cast<int>(fd));
  if (!description) return fkernel::return_error(fk::core::Error::InvalidHandle);

  // Basic TTY ioctl stubs
  if (request == TCGETS || request == TCSETS || request == TIOCGWINSZ) {
      if (request == TIOCGWINSZ && arg) {
          struct winsize* ws = reinterpret_cast<struct winsize*>(arg);
          ws->ws_row = 25;
          ws->ws_col = 80;
      }
      return 0; // Pretend success
  }

  auto res = description->ioctl(request, arg);
  if (res.is_error()) return fkernel::return_error(res.error());

  return res.value();
}
}
