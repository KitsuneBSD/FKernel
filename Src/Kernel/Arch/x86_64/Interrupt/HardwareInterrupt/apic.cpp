#include <Kernel/Arch/x86_64/Interrupt/HardwareInterrupts/apic.h>
#include <Kernel/Hardware/Cpu.h>

auto cpu_instance = CPU::the();

void APIC::enable() {
  bool is_present = CPU::the().has_apic();

  if (!is_present) {
    kerror("APIC", "Local APIC not detected");
    return;
  }

  uint64_t apic_msr = cpu_instance.read_msr(APIC_BASE_MSR);
  apic_msr |= APIC_ENABLE;
  uintptr_t apic_phys = apic_msr & 0xFFFFF000;

  this->lapic = reinterpret_cast<local_apic *>(apic_phys);

  klog("APIC", "Local APIC enabled at phys: 0x%lx", apic_phys);
}

void APIC::send_eoi() { this->lapic->eoi = 0; }
