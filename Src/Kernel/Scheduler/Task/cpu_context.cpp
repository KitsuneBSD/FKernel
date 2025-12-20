#include <Kernel/Scheduler/Task/cpu_context.h>
#include <Kernel/Hardware/Cpu/cpu.h>
#include <Kernel/Hardware/Cpu/cpu_register.h>

CpuContext GetContextForNewTask(
    void (*entry)(),
    uint64_t stack_top,
    bool is_kernel_task
) {
    (void)is_kernel_task;
     uint64_t* stack = reinterpret_cast<uint64_t*>(stack_top);
    *(--stack) = reinterpret_cast<uint64_t>(entry);

    CpuContext ctx{
        .r15 = CPU::the().read_register(CpuRegister::R15),
        .r14 = CPU::the().read_register(CpuRegister::R14),
        .r13 = CPU::the().read_register(CpuRegister::R13),
        .r12 = CPU::the().read_register(CpuRegister::R12),
        .r11 = CPU::the().read_register(CpuRegister::R11),
        .r10 = CPU::the().read_register(CpuRegister::R10),
        .r9  = CPU::the().read_register(CpuRegister::R9),
        .r8  = CPU::the().read_register(CpuRegister::R8),

        .rdi = CPU::the().read_register(CpuRegister::RDI),
        .rsi = CPU::the().read_register(CpuRegister::RSI),
        .rbp = reinterpret_cast<uint64_t>(stack),
        .rdx = CPU::the().read_register(CpuRegister::RDX),
        .rcx = CPU::the().read_register(CpuRegister::RCX),
        .rbx = CPU::the().read_register(CpuRegister::RBX),
        .rax = CPU::the().read_register(CpuRegister::RAX),
    };

    return ctx;
}
