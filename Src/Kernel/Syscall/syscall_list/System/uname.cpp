#include <Kernel/Arch/x86_64/Syscall/syscall_arch.h>
#include <Kernel/Syscall/syscall.h>
#include <Kernel/Syscall/syscall_utils.h>
#include <LibFK/Utilities/memory.h>

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
  fk::memory::copy_string(buf->sysname, "FKernel");
  fk::memory::copy_string(buf->nodename, "fkernel-machine");
  fk::memory::copy_string(buf->release,
         "0.0.1-fkernel"); // Versão moderna para encorajar getdents64
  fk::memory::copy_string(buf->version, "#1 SMP Friday, Jan 23 2026");
  fk::memory::copy_string(buf->machine, "x86_64");
  fk::memory::copy_string(buf->domainname, "(none)");

  return 0;
}
}
