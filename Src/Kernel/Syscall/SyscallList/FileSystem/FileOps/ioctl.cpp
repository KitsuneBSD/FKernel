#include <LibFK/Utilities/memory.h>

#include <Kernel/Arch/x86_64/Syscall/syscall_arch.h>
#include <Kernel/Driver/Terminal/terminal_manager.h>
#include <Kernel/Fs/Vfs/Core/virtual_filesystem.h>
#include <Kernel/Memory/UserAccess/user_access.h>
#include <Kernel/Scheduler/Core/scheduler.h>
#include <Kernel/Syscall/syscall_utils.h>

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

  auto res = description->ioctl(request, arg);
  if (res.is_ok()) return res.value();

  // TCGETS/TCSETS on a non-terminal node → ENOTTY
  if (request == TCGETS || request == TCSETS || request == TCSETSW || request == TCSETSF)
      return fkernel::return_error(fk::core::Error::InappropriateIoctlForDevice);

  // TIOCGPGRP fallback: return current task's pgid
  if (request == TIOCGPGRP && arg) {
      if (!fkernel::memory::is_user_address(arg, sizeof(int))) return -14; // EFAULT
      int pgid = static_cast<int>(task->control.identity.pgid.value());
      auto wr = fkernel::memory::copy_to_user(reinterpret_cast<void*>(arg), &pgid, sizeof(pgid));
      if (wr.is_error()) return -14; // EFAULT
      return 0;
  }

  // TIOCGWINSZ: query real terminal dimensions via TerminalManager
  if (request == TIOCGWINSZ && arg) {
      if (!fkernel::memory::is_user_address(arg, sizeof(struct winsize))) return -14; // EFAULT
      struct winsize kws;
      auto* terminal = fkernel::terminal::TerminalManager::the().active_terminal();
      if (terminal) {
          kws.ws_col    = static_cast<unsigned short>(terminal->get_width());
          kws.ws_row    = static_cast<unsigned short>(terminal->get_height());
      } else {
          kws.ws_col = 80;
          kws.ws_row = 25;
      }
      kws.ws_xpixel = 0;
      kws.ws_ypixel = 0;
      auto wr = fkernel::memory::copy_to_user(reinterpret_cast<void*>(arg), &kws, sizeof(kws));
      if (wr.is_error()) return -14; // EFAULT
      return 0;
  }

  return fkernel::return_error(res.error());
}
}