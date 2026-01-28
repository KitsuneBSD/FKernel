#include <Kernel/Syscall/syscall_utils.h>
#include <Kernel/Arch/x86_64/Syscall/syscall_arch.h>
#include <Kernel/Fs/Vfs/virtual_filesystem.h>
#include <Kernel/Scheduler/scheduler.h>
#include <LibC/string.h>

struct winsize {
    unsigned short ws_row;
    unsigned short ws_col;
    unsigned short ws_xpixel;
    unsigned short ws_ypixel;
};

#define TCGETS 0x5401
#define TCSETS 0x5402
#define TCSETSW 0x5403
#define TCSETSF 0x5404
#define TIOCGWINSZ 0x5413
#define TIOCSWINSZ 0x5414
#define TIOCGPGRP 0x540F
#define TIOCSPGRP 0x5410

extern "C" {

uint64_t sys_ioctl(uint64_t fd, uint64_t request, uint64_t arg, uint64_t, uint64_t, uint64_t, [[maybe_unused]] PtRegs* regs) {
  auto *task = SchedulerManager::the().current();
  if (!task) return fkernel::return_error(fk::core::Error::PermissionDenied);

  auto description = task->get_file_descriptor(static_cast<int>(fd));
  if (!description) return fkernel::return_error(fk::core::Error::InvalidHandle);

  // Delegate to the underlying node/device
  auto res = description->ioctl(request, arg);
  if (res.is_ok()) return res.value();

  // Basic TTY ioctl stubs fallback if not handled by node
  if (request == TCGETS || request == TCSETS || request == TCSETSW || request == TCSETSF || 
      request == TIOCGWINSZ || request == TIOCSWINSZ || request == TIOCGPGRP || request == TIOCSPGRP) {
      
      if (request == TIOCGWINSZ && arg) {
          struct winsize* ws = reinterpret_cast<struct winsize*>(arg);
          ws->ws_row = 25;
          ws->ws_col = 80;
      }
      
      if (request == TIOCGPGRP && arg) {
          *reinterpret_cast<int*>(arg) = static_cast<int>(task->id); // Simplified
      }

      return 0; // Pretend success
  }

  fk::algorithms::kdebug("SYSCALL", "sys_ioctl: Unhandled request 0x%lx for FD %lu", request, fd);
  return fkernel::return_error(res.error());
}
}
