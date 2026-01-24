#include <Kernel/Hardware/Cpu/cpu.h>
#include <Kernel/Hardware/Cpu/cpu_context.h>
#include <Kernel/Hardware/Cpu/cpu_register.h>

CpuContext GetContextForNewTask(uint64_t stack_pointer, bool is_kernel_task,
                                uint64_t arg1, uint64_t arg2) {
  CpuContext ctx{
      .rip = 0,
      .cs = is_kernel_task ? 0x08 : 0x23,
      .rflags = 0x202,
      .rsp = stack_pointer,
      .ss = is_kernel_task ? 0x10 : 0x1B,

      .padding = 0,

      .r15 = CPU::the().read_register(CpuRegister::R15),
      .r14 = CPU::the().read_register(CpuRegister::R14),
      .r13 = CPU::the().read_register(CpuRegister::R13),
      .r12 = CPU::the().read_register(CpuRegister::R12),
      .rbp = stack_pointer,
      .rbx = CPU::the().read_register(CpuRegister::RBX),

      .r11 = CPU::the().read_register(CpuRegister::R11),
      .r10 = CPU::the().read_register(CpuRegister::R10),
      .r9 = CPU::the().read_register(CpuRegister::R9),
      .r8 = CPU::the().read_register(CpuRegister::R8),
      .rdi = arg1,
      .rsi = arg2,
      .rdx = CPU::the().read_register(CpuRegister::RDX),
      .rcx = CPU::the().read_register(CpuRegister::RCX),
      .rax = CPU::the().read_register(CpuRegister::RAX),
  };

  return ctx;
}
