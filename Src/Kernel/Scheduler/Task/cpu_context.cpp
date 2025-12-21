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

        .rip = reinterpret_cast<uint64_t>(entry),
        .cs = 0x08,
        .rflags = 0x202,
        .rsp = reinterpret_cast<uint64_t>(stack),
        .ss = 0x10
    };

    return ctx;
}

void SaveContext(CpuContext& ctx, const InterruptFrame& frame) {
    ctx.rax = frame.rax;
    ctx.rbx = frame.rbx;
    ctx.rcx = frame.rcx;
    ctx.rdx = frame.rdx;
    ctx.rsi = frame.rsi;
    ctx.rdi = frame.rdi;
    ctx.rbp = frame.rbp;
    ctx.r8  = frame.r8;
    ctx.r9  = frame.r9;
    ctx.r10 = frame.r10;
    ctx.r11 = frame.r11;
    ctx.r12 = frame.r12;
    ctx.r13 = frame.r13;
    ctx.r14 = frame.r14;
    ctx.r15 = frame.r15;

    ctx.rip    = frame.rip;
    ctx.rflags = frame.rflags;
}

void LoadContext(InterruptFrame& frame, const CpuContext& ctx) {
    frame.rax = ctx.rax;
    frame.rbx = ctx.rbx;
    frame.rcx = ctx.rcx;
    frame.rdx = ctx.rdx;
    frame.rsi = ctx.rsi;
    frame.rdi = ctx.rdi;
    frame.rbp = ctx.rbp;
    frame.r8  = ctx.r8;
    frame.r9  = ctx.r9;
    frame.r10 = ctx.r10;
    frame.r11 = ctx.r11;
    frame.r12 = ctx.r12;
    frame.r13 = ctx.r13;
    frame.r14 = ctx.r14;
    frame.r15 = ctx.r15;

    frame.rip    = ctx.rip;
    frame.rflags = ctx.rflags;
}
