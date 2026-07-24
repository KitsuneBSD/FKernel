#include <Kernel/Hardware/Cpu/cpu.h>
#include <Kernel/Hardware/Cpu/cpu_register.h>

uint64_t CPU::read_register(CpuRegister reg) {
    uint64_t value = 0;

    switch (reg) {
        case CpuRegister::RAX:
            asm volatile("mov %%rax, %0" : "=r"(value));
            break;
        case CpuRegister::RBX:
            asm volatile("mov %%rbx, %0" : "=r"(value));
            break;
        case CpuRegister::RCX:
            asm volatile("mov %%rcx, %0" : "=r"(value));
            break;
        case CpuRegister::RDX:
            asm volatile("mov %%rdx, %0" : "=r"(value));
            break;
        case CpuRegister::RSI:
            asm volatile("mov %%rsi, %0" : "=r"(value));
            break;
        case CpuRegister::RDI:
            asm volatile("mov %%rdi, %0" : "=r"(value));
            break;
        case CpuRegister::RBP:
            asm volatile("mov %%rbp, %0" : "=r"(value));
            break;
        case CpuRegister::R8:
            asm volatile("mov %%r8,  %0" : "=r"(value));
            break;
        case CpuRegister::R9:
            asm volatile("mov %%r9,  %0" : "=r"(value));
            break;
        case CpuRegister::R10:
            asm volatile("mov %%r10, %0" : "=r"(value));
            break;
        case CpuRegister::R11:
            asm volatile("mov %%r11, %0" : "=r"(value));
            break;
        case CpuRegister::R12:
            asm volatile("mov %%r12, %0" : "=r"(value));
            break;
        case CpuRegister::R13:
            asm volatile("mov %%r13, %0" : "=r"(value));
            break;
        case CpuRegister::R14:
            asm volatile("mov %%r14, %0" : "=r"(value));
            break;
        case CpuRegister::R15:
            asm volatile("mov %%r15, %0" : "=r"(value));
            break;
    }

    return value;
}

void CPU::write_register(CpuRegister reg, uint64_t value) {
    switch (reg) {
        case CpuRegister::RAX:
            asm volatile("mov %0, %%rax" :: "r"(value));
            break;
        case CpuRegister::RBX:
            asm volatile("mov %0, %%rbx" :: "r"(value));
            break;
        case CpuRegister::RCX:
            asm volatile("mov %0, %%rcx" :: "r"(value));
            break;
        case CpuRegister::RDX:
            asm volatile("mov %0, %%rdx" :: "r"(value));
            break;
        case CpuRegister::RSI:
            asm volatile("mov %0, %%rsi" :: "r"(value));
            break;
        case CpuRegister::RDI:
            asm volatile("mov %0, %%rdi" :: "r"(value));
            break;
        case CpuRegister::RBP:
            asm volatile("mov %0, %%rbp" :: "r"(value));
            break;
        case CpuRegister::R8:
            asm volatile("mov %0, %%r8"  :: "r"(value));
            break;
        case CpuRegister::R9:
            asm volatile("mov %0, %%r9"  :: "r"(value));
            break;
        case CpuRegister::R10:
            asm volatile("mov %0, %%r10" :: "r"(value));
            break;
        case CpuRegister::R11:
            asm volatile("mov %0, %%r11" :: "r"(value));
            break;
        case CpuRegister::R12:
            asm volatile("mov %0, %%r12" :: "r"(value));
            break;
        case CpuRegister::R13:
            asm volatile("mov %0, %%r13" :: "r"(value));
            break;
        case CpuRegister::R14:
            asm volatile("mov %0, %%r14" :: "r"(value));
            break;
        case CpuRegister::R15:
            asm volatile("mov %0, %%r15" :: "r"(value));
            break;
    }
}