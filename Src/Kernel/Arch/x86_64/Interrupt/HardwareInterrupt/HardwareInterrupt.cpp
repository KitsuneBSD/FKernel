#include <Kernel/Arch/x86_64/Interrupt/HardwareInterrupts/HardwareInterrupt.h>
#include <Kernel/Arch/x86_64/Interrupt/HardwareInterrupts/InterruptController/8259_pic.h>
#include <Kernel/Arch/x86_64/Interrupt/HardwareInterrupts/InterruptController/apic.h>
#include <Kernel/Arch/x86_64/Interrupt/HardwareInterrupts/InterruptController/ioapic.h>
#include <Kernel/Arch/x86_64/Interrupt/HardwareInterrupts/InterruptController/x2apic.h>
#include <Kernel/Arch/x86_64/Interrupt/HardwareInterrupts/TimerInterrupt.h>
#include <Kernel/Hardware/cpu.h>

void HardwareInterruptManager::initialize() {
  static PIC8259 pic;
  if (CPU::the().has_x2apic() && m_has_memory_manager) {
    static X2APIC x2apic;
    static IOAPIC ioapic;

    fk::algorithms::klog("HW_INTERRUPT", "Enable x2APIC + IOAPIC");
    x2apic.initialize();
    x2apic.setup_timer(1);
    pic.disable();

    ioapic.initialize();
    m_controller = &ioapic;
  } else if (CPU::the().has_apic() && m_has_memory_manager) {
    static APIC apic;
    static IOAPIC ioapic;

    fk::algorithms::klog("HW_INTERRUPT", "Enable APIC + IOAPIC");
    apic.initialize();
    apic.calibrate_timer();
    apic.setup_timer(1);
    pic.disable();

    ioapic.initialize();
    m_controller = &ioapic;
  } else {
    fk::algorithms::klog("HW_INTERRUPT", "Enable PIC8259");
    pic.initialize();
    m_controller = &pic;
  }

  TimerManager::the().initialize(100);
}

void HardwareInterruptManager::mask_interrupt(uint8_t irq) {
  if (m_controller)
    m_controller->mask_interrupt(irq);
}

void HardwareInterruptManager::unmask_interrupt(uint8_t irq) {
  if (m_controller)
    m_controller->unmask_interrupt(irq);
}

void HardwareInterruptManager::send_eoi(uint8_t irq) {
  if (m_controller)
    m_controller->send_eoi(irq);
}
