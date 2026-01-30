#include <Kernel/Arch/x86_64/Syscall/syscall_arch.h>
#include <Kernel/Hardware/Cpu/cpu.h>
#include <Kernel/Scheduler/scheduler.h>
#include <Kernel/Syscall/syscall.h>
#include <LibFK/Algorithms/log.h>

static bool is_canonical(uint64_t addr) {
  return ((int64_t)addr >> 47) == 0 || ((int64_t)addr >> 47) == -1;
}

extern "C" uint64_t sys_arch_prctl(uint64_t code, uint64_t addr, uint64_t,
                                   uint64_t, uint64_t, uint64_t, [[maybe_unused]] PtRegs* regs) {
  auto* task = SchedulerManager::the().current();
  if (!task) return -1;
  
  if (code == 0x1002 || code == 0x1001) {
    if (!is_canonical(addr)) {
      fk::algorithms::kwarn("SYSCALL", "sys_arch_prctl: non-canonical address 0x%lx", addr);
      return -1;
    }
  }

  switch (code) {

  case 0x1002: // ARCH_SET_FS
    if (task)
      task->resources.context.fs_base = addr;
    CPU::the().write_msr(MSR_FS_BASE, addr);
    return 0;
    case 0x1001: // ARCH_SET_GS
      if (task) task->resources.context.gs_base = addr;
      CPU::the().write_msr(MSR_KERNEL_GS_BASE, addr);
      return 0;
    case 0x1003: // ARCH_GET_FS
      *reinterpret_cast<uint64_t *>(addr) = CPU::the().read_msr(MSR_FS_BASE);
      return 0;
    case 0x1004: // ARCH_GET_GS
      *reinterpret_cast<uint64_t *>(addr) = CPU::the().read_msr(MSR_KERNEL_GS_BASE);
      return 0;
  
  default:
    fk::algorithms::kwarn("SYSCALL", "sys_arch_prctl: unknown code 0x%lx",
                          code);
    return -1;
  }
}