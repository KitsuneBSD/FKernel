#include <Kernel/Arch/x86_64/arch_defs.h>
#include <Kernel/Arch/x86_64/Interrupt/HardwareInterrupts/apic.h>
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

void APIC::send_eoi() {
  if(this->lapic)
    this->lapic->eoi = 0; 
}
