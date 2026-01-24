#include <Kernel/Arch/x86_64/Syscall/syscall_arch.h>
#include <Kernel/Hardware/Cpu/cpu.h>
#include <Kernel/Syscall/syscall.h>
#include <LibFK/Algorithms/log.h>

extern "C" uint64_t sys_arch_prctl(uint64_t code, uint64_t addr, uint64_t,
                                   uint64_t, uint64_t, uint64_t) {
  switch (code) {
  case 0x1002: // ARCH_SET_FS
    CPU::the().write_msr(MSR_FS_BASE, addr);
    return 0;
  case 0x1001: // ARCH_SET_GS
    CPU::the().write_msr(MSR_GS_BASE, addr);
    return 0;
  case 0x1003: // ARCH_GET_FS
    *reinterpret_cast<uint64_t *>(addr) = CPU::the().read_msr(MSR_FS_BASE);
    return 0;
  case 0x1004: // ARCH_GET_GS
    *reinterpret_cast<uint64_t *>(addr) = CPU::the().read_msr(MSR_GS_BASE);
    return 0;
  default:
    fk::algorithms::kwarn("SYSCALL", "sys_arch_prctl: unknown code 0x%lx",
                          code);
    return -1;
  }
}
