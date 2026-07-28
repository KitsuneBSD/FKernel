#include <Kernel/Arch/x86_64/Syscall/syscall_arch.h>
#include <Kernel/Arch/x86_64/arch_defs.h>
#include <Kernel/Arch/x86_64/io.h>
#include <Kernel/Hardware/Cpu/cpu.h>
#include <Kernel/Hardware/Cpu/cpu_block.h>
#include <LibFK/Algorithms/log.h>

extern "C" void syscall_stub();
extern "C" uint64_t stack_top;
extern "C" uint64_t syscall_kernel_stack;

#define MSR_KERNEL_GS_BASE 0xC0000102

CpuControlBlock g_cpu_block;

void init_syscalls(size_t) {
  g_cpu_block.kernel_stack = (uint64_t)&stack_top;
  g_cpu_block.user_rsp = 0;
  g_cpu_block.cpu_id = 0;
  g_cpu_block.current_task = nullptr;

  CPU::the().write_msr(MSR_GS_BASE, (uint64_t)&g_cpu_block);
  CPU::the().write_msr(MSR_KERNEL_GS_BASE, 0);

  syscall_kernel_stack = g_cpu_block.kernel_stack;

  uint64_t efer = CPU::the().read_msr(MSR_EFER);
  if (!(efer & EFER_SCE)) {
    CPU::the().write_msr(MSR_EFER, efer | EFER_SCE);
  }

  uint64_t star = ((uint64_t)0x10 << 48) | ((uint64_t)0x08 << 32);
  CPU::the().write_msr(MSR_STAR, star);

  CPU::the().write_msr(MSR_LSTAR, (uint64_t)syscall_stub);

  CPU::the().write_msr(MSR_SFMASK, (uint64_t)0x200);

  fk::algorithms::klog("SYSCALL", "Initialized SYSCALL/SYSRET MSRs and GS Base");
}
