#include <Kernel/Arch/x86_64/Syscall/syscall_arch.h>
#include <Kernel/Arch/x86_64/io.h>
#include <Kernel/Hardware/Cpu/cpu.h>
#include <Kernel/Hardware/Cpu/cpu_block.h>
#include <LibFK/Algorithms/log.h>

extern "C" void syscall_stub();
extern "C" uint64_t stack_top;
extern "C" uint64_t syscall_kernel_stack;

// Global instance for single-core (TODO: Multi-core array)
CpuControlBlock g_cpu_block;

// MSR for Kernel GS Base
#define MSR_KERNEL_GS_BASE 0xC0000102

void init_syscalls() {
  // Initialize Global CPU Block
  g_cpu_block.kernel_stack = (uint64_t)&stack_top;
  g_cpu_block.user_rsp = 0;
  g_cpu_block.cpu_id = 0;
  g_cpu_block.current_task = nullptr;

  // Set MSR_KERNEL_GS_BASE to point to our block
  CPU::the().write_msr(MSR_KERNEL_GS_BASE, (uint64_t)&g_cpu_block);

  // Also initialize existing global for compatibility during transition
  syscall_kernel_stack = (uint64_t)&stack_top;

  uint64_t efer = CPU::the().read_msr(MSR_EFER);
  if (!(efer & EFER_SCE)) {
    CPU::the().write_msr(MSR_EFER, efer | EFER_SCE);
  }

  uint64_t star = ((uint64_t)0x10 << 48) | ((uint64_t)0x08 << 32);
  CPU::the().write_msr(MSR_STAR, star);

  CPU::the().write_msr(MSR_LSTAR, (uint64_t)syscall_stub);

  CPU::the().write_msr(MSR_SFMASK, (uint64_t)0x200);

  fk::algorithms::klog("SYSCALL",
                       "Initialized SYSCALL/SYSRET MSRs and GS Base");
}
