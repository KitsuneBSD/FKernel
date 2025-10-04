#include <Kernel/Arch/x86_64/arch_defs.h>
#include <Kernel/Arch/x86_64/Interrupt/HardwareInterrupts/apic.h>
#include <Kernel/Arch/x86_64/Interrupt/HardwareInterrupts/pit.h>
#include <Kernel/MemoryManager/VirtualMemoryManager.h>
#include <Kernel/Hardware/Cpu.h>

auto cpu_instance = CPU::the();

void APIC::enable() {
  if (!CPU::the().has_apic()) {
    kerror("APIC", "Local APIC not detected");
    return;
  }

  uint64_t apic_msr = CPU::the().read_msr(APIC_BASE_MSR);
  uintptr_t apic_phys = apic_msr & 0xFFFFF000;

  apic_msr |= APIC_ENABLE;
  CPU::the().write_msr(APIC_BASE_MSR, apic_msr);

 for (uintptr_t offset = 0; offset < APIC_RANGE_SIZE; offset += PAGE_SIZE) {
        VirtualMemoryManager::the().map_page(
            apic_phys + offset,
            apic_phys + offset,
            PageFlags::Present | PageFlags::Writable | PageFlags::WriteThrough
        );
    }

  lapic = reinterpret_cast<local_apic *>(apic_phys);

  lapic->spurious = APIC_SPURIOUS | APIC_SVR_ENABLE;

  klog("APIC", "Mapped APIC range [0x%lx - 0x%lx]", apic_phys, apic_phys + APIC_RANGE_SIZE);
}

void APIC::calibrate_timer() {
  if (!this->lapic)
    return; 
  
  lapic->divide_config = APIC_TIMER_DIVISOR;
  lapic->lvt_timer = 0x10000;

  lapic->initial_count = 0xFFFFFFFF;

  PIT::the().sleep(10);

  uint64_t elapsed = 0xFFFFFFFF - lapic->current_count;

  apic_ticks_per_ms = elapsed / 10;

  klog("APIC", "Timer calibrated: %lu ticks/ms", apic_ticks_per_ms);
}

void APIC::setup_timer(uint64_t ms) {
    if (!lapic) return;
    if (apic_ticks_per_ms == 0)
        calibrate_timer();

    uint64_t initial = apic_ticks_per_ms * ms;

    lapic->divide_config = APIC_TIMER_DIVISOR;
    lapic->lvt_timer = 0x20 | APIC_LVT_TIMER_MODE_PERIODIC;
    lapic->initial_count = initial;

    klog("APIC", "Timer set: %lu ms (%lu ticks)", ms, initial);
}

void APIC::send_eoi() {
  if(this->lapic)
    this->lapic->eoi = 0; 
}
