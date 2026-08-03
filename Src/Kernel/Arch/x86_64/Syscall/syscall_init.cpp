#include <Kernel/Arch/x86_64/Syscall/syscall_arch.h>
#include <Kernel/Arch/x86_64/arch_defs.h>
#include <Kernel/Arch/x86_64/io.h>
#include <Kernel/Hardware/Cpu/cpu.h>
#include <Kernel/Hardware/Cpu/cpu_block.h>
#include <LibFK/Algorithms/Logging/log.h>

extern "C" void syscall_stub();
extern "C" uint64_t stack_top;

#define MSR_KERNEL_GS_BASE 0xC0000102

CpuControlBlock g_cpu_blocks[MAX_CPUS];

void init_syscalls(size_t cpu_index) {
  if (cpu_index >= MAX_CPUS) cpu_index = 0;

  CpuControlBlock& block = g_cpu_blocks[cpu_index];
  block.kernel_stack = (uint64_t)&stack_top;
  block.user_rsp = 0;
  block.cpu_id = cpu_index;
  block.current_task = nullptr;

  CPU::the().write_msr(MSR_GS_BASE, (uint64_t)&block);
  CPU::the().write_msr(MSR_KERNEL_GS_BASE, 0);

  uint64_t efer = CPU::the().read_msr(MSR_EFER);
  if (!(efer & EFER_SCE)) {
    CPU::the().write_msr(MSR_EFER, efer | EFER_SCE);
  }

  uint64_t star = ((uint64_t)0x10 << 48) | ((uint64_t)0x08 << 32);
  CPU::the().write_msr(MSR_STAR, star);

  CPU::the().write_msr(MSR_LSTAR, (uint64_t)syscall_stub);

  // Clear TF(8)+IF(9)+DF(10)+NT(14)+AC(18) on syscall entry — matches Linux 0x47700
  CPU::the().write_msr(MSR_SFMASK, (uint64_t)0x47700);

  fk::algorithms::klog("SYSCALL", "CPU %zu: SYSCALL/SYSRET MSRs and GS Base initialized", cpu_index);
}
