#include <Kernel/Arch/x86_64/Smp/ap_entry.h>
#include <Kernel/Arch/x86_64/Interrupt/HardwareInterrupts/InterruptController/apic.h>
#include <Kernel/Arch/x86_64/Interrupt/HardwareInterrupts/InterruptController/apic_common.h>
#include <Kernel/Arch/x86_64/Interrupt/HardwareInterrupts/InterruptController/x2apic.h>
#include <Kernel/Arch/x86_64/Segments/gdt.h>
#include <Kernel/Arch/x86_64/arch_defs.h>
#include <Kernel/Hardware/Cpu/cpu.h>
#include <Kernel/Scheduler/scheduler.h>
#include <LibFK/Algorithms/log.h>

extern void init_syscalls(size_t cpu_index);

extern "C" void ap_entry(uint32_t cpu_index) {
    GDTController::the().load_per_cpu(cpu_index);

    // Set up per-CPU GS base and SYSCALL MSRs for this AP.
    init_syscalls(cpu_index);

    if (CPU::the().has_x2apic()) {
        X2APIC::the().calibrate_timer();
        X2APIC::the().setup_timer(100);
    } else {
        APIC::the().calibrate_timer();
        APIC::the().setup_timer(100);
    }

    fk::algorithms::klog("SMP", "CPU %u online", cpu_index);

    auto* trampoline_data = reinterpret_cast<fkernel::smp::TrampolineData*>(
        fkernel::smp::TRAMPOLINE_PHYS_ADDR + 0xF80);
    __sync_synchronize();
    trampoline_data->online_flag = 1;

    SchedulerManager::the().idle_loop();
}
