#include <Kernel/Arch/x86_64/Syscall/syscall_arch.h>
#include <Kernel/Syscall/syscall.h>
#include <Kernel/Syscall/syscall_utils.h>
#include <LibC/string.h>

struct utsname {
  char sysname[65];
  char nodename[65];
  char release[65];
  char version[65];
  char machine[65];
  char domainname[65];
};

extern "C" {
uint64_t sys_uname(uint64_t buf_ptr, uint64_t, uint64_t, uint64_t, uint64_t,
                   uint64_t, [[maybe_unused]] PtRegs* regs) {
  if (!buf_ptr)
    return -22; // EINVAL

  utsname *buf = reinterpret_cast<utsname *>(buf_ptr);
  strcpy(buf->sysname, "FKernel");
  strcpy(buf->nodename, "fkernel-machine");
  strcpy(buf->release,
         "0.0.1-fkernel"); // Versão moderna para encorajar getdents64
  strcpy(buf->version, "#1 SMP Friday, Jan 23 2026");
  strcpy(buf->machine, "x86_64");
  strcpy(buf->domainname, "(none)");

  return 0;
}
}
