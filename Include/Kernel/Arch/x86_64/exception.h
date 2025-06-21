#pragma once

#include <LibC/stdint.h>
#include <LibFK/Log.hpp>

namespace exception {

struct [[gnu::packed]] CPUState {
    LibC::uint64_t r15;
    LibC::uint64_t r14;
    LibC::uint64_t r13;
    LibC::uint64_t r12;
    LibC::uint64_t r11;
    LibC::uint64_t r10;
    LibC::uint64_t r9;
    LibC::uint64_t r8;
    LibC::uint64_t rbp;
    LibC::uint64_t rdi;
    LibC::uint64_t rsi;
    LibC::uint64_t rdx;
    LibC::uint64_t rcx;
    LibC::uint64_t rbx;
    LibC::uint64_t rax;

    LibC::uint64_t vector;
    LibC::uint64_t error_code; // zero se não existir

    LibC::uint64_t rip;
    LibC::uint64_t cs;
    LibC::uint64_t rflags;
    LibC::uint64_t rsp;
    LibC::uint64_t ss;
};

enum class Vector : LibC::uint8_t {
    DivideByZero = 0,                // #DE Divide Error
    Debug = 1,                       // #DB Debug Exception
    NMI = 2,                         // Non-Maskable Interrupt
    Breakpoint = 3,                  // #BP Breakpoint
    Overflow = 4,                    // #OF Overflow
    BoundRangeExceeded = 5,          // #BR BOUND Range Exceeded
    InvalidOpcode = 6,               // #UD Invalid Opcode
    DeviceNotAvailable = 7,          // #NM Device Not Available (No Math Coprocessor)
    DoubleFault = 8,                 // #DF Double Fault
    CoprocessorSegmentOverrun = 9,   // Reserved (Intel reserved)
    InvalidTSS = 10,                 // #TS Invalid TSS
    SegmentNotPresent = 11,          // #NP Segment Not Present
    StackSegmentFault = 12,          // #SS Stack-Segment Fault
    GeneralProtection = 13,          // #GP General Protection Fault
    PageFault = 14,                  // #PF Page Fault
    Reserved15 = 15,                 // Intel reserved
    x87FloatingPointException = 16,  // #MF x87 FPU Floating-Point Error
    AlignmentCheck = 17,             // #AC Alignment Check
    MachineCheck = 18,               // #MC Machine Check
    SIMDException = 19,              // #XM SIMD Floating-Point Exception
    VirtualizationException = 20,    // #VE Virtualization Exception
    ControlProtectionException = 21, // #CP Control Protection Exception
    Reserved22 = 22,
    Reserved23 = 23,
    Reserved24 = 24,
    Reserved25 = 25,
    Reserved26 = 26,
    Reserved27 = 27,
    HypervisorInjectionException = 28, // #HV Hypervisor Injection Exception
    VMMCommunicationException = 29,    // #VC VMM Communication Exception
    SecurityException = 30,            // #SX Security Exception
    Reserved31 = 31
};

void halt();
char const* vector_to_string(Vector vec);
extern "C" void default_exception_handler(exception::CPUState const* frame);

inline void log_stacktrace(CPUState const* state, int max_frames = 10) noexcept
{
    Log(LogLevel::WARN, "=== CPU Exception Stacktrace ===");

    LibC::uintptr_t rip = state->rip;
    LibC::uintptr_t rbp = state->rbp;

    Logf(LogLevel::WARN, "RIP: 0x%016llx\n", rip);

    for (int frame = 0; frame < max_frames; ++frame) {
        if (rbp == 0) {
            Log(LogLevel::WARN, "[End of stack frames]\n");
            break;
        }

        if (rbp & 0x7) {
            Logf(LogLevel::WARN, "Unaligned RBP detected: 0x%016llx\n", rbp);
            break;
        }

        LibC::uintptr_t* frame_ptr = reinterpret_cast<LibC::uintptr_t*>(rbp);
        LibC::uintptr_t ret_addr = *(frame_ptr + 1);

        Logf(LogLevel::WARN, "#%d RIP = 0x%016llx (return address)\n", frame, ret_addr);

        rbp = *frame_ptr;

        if (ret_addr == 0)
            break;
    }
}

inline void log_cpu_state_warn(CPUState const* state) noexcept
{
    Logf(LogLevel::WARN, "[WARN] CPU Exception State:\n");
    Logf(LogLevel::WARN, "Vector: %llu\n", state->vector);
    log_stacktrace(state);
}
}
