#include "LibFK/Log.hpp"
#include <Kernel/Arch/x86_64/exception.h>
#include <Kernel/Arch/x86_64/stack_size.h>
#include <LibC/stdint.h>

namespace exception {

extern void (*const isr_stubs[32])();
extern LibC::uint8_t const isr_ist[32];

char const* vector_to_string(Vector vec)
{
    switch (vec) {
    case Vector::DivideByZero:
        return "Divide By Zero";
    case Vector::Debug:
        return "Debug";
    case Vector::NMI:
        return "Non-Maskable Interrupt";
    case Vector::Breakpoint:
        return "Breakpoint";
    case Vector::Overflow:
        return "Overflow";
    case Vector::BoundRangeExceeded:
        return "Bound Range Exceeded";
    case Vector::InvalidOpcode:
        return "Invalid Opcode";
    case Vector::DeviceNotAvailable:
        return "Device Not Available";
    case Vector::DoubleFault:
        return "Double Fault";
    case Vector::CoprocessorSegmentOverrun:
        return "Coprocessor Segment Overrun";
    case Vector::InvalidTSS:
        return "Invalid TSS";
    case Vector::SegmentNotPresent:
        return "Segment Not Present";
    case Vector::StackSegmentFault:
        return "Stack Segment Fault";
    case Vector::GeneralProtection:
        return "General Protection Fault";
    case Vector::PageFault:
        return "Page Fault";
    case Vector::x87FloatingPointException:
        return "x87 Floating Point Exception";
    case Vector::AlignmentCheck:
        return "Alignment Check";
    case Vector::MachineCheck:
        return "Machine Check";
    case Vector::SIMDException:
        return "SIMD Floating Point Exception";
    case Vector::VirtualizationException:
        return "Virtualization Exception";
    case Vector::ControlProtectionException:
        return "Control Protection Exception";
    case Vector::HypervisorInjectionException:
        return "Hypervisor Injection Exception";
    case Vector::VMMCommunicationException:
        return "VMM Communication Exception";
    case Vector::SecurityException:
        return "Security Exception";
    default:
        return "Unknown Exception";
    }
}

void halt()
{
    while (true) {
        asm("cli;hlt");
    }
}

inline LibC::uint64_t read_cr2()
{
    LibC::uint64_t cr2;
    asm volatile("mov %%cr2, %0" : "=r"(cr2));
    return cr2;
}

extern "C" void default_exception_handler(exception::CPUState const* frame)
{
    Logf(LogLevel::ERROR, "Exception %s occurred", exception::vector_to_string(static_cast<Vector>(frame->vector)));
    log_cpu_state_warn(frame);
    exception::halt();
}
}
