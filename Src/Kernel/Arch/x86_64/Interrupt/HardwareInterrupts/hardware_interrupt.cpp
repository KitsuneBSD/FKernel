#include <Kernel/Arch/x86_64/Interrupt/HardwareInterrupts/hardware_interrupt.h>
#include <Kernel/Arch/x86_64/Interrupt/HardwareInterrupts/InterruptController/8259_pic.h>
#include <Kernel/Arch/x86_64/Interrupt/HardwareInterrupts/InterruptController/apic.h>
#include <Kernel/Arch/x86_64/Interrupt/HardwareInterrupts/InterruptController/ioapic.h>
#include <Kernel/Arch/x86_64/Interrupt/HardwareInterrupts/InterruptController/x2apic.h>
#include <Kernel/Arch/x86_64/Interrupt/HardwareInterrupts/timer_interrupt.h>
#include <Kernel/Hardware/Cpu/cpu.h>

void HardwareInterruptManager::select_and_configure_controller() {
  static PIC8259 pic_controller_instance;
  static APIC apic_controller_instance;
  static IOAPIC ioapic_controller_instance;
  static X2APIC x2apic_controller_instance;

  HardwareInterrupt *new_controller = nullptr;
  fk::text::String controller_name = "None";

  if (CPU::the().has_x2apic() && m_has_memory_manager) {
    fk::algorithms::klog("HW_INTERRUPT", "Enabling x2APIC + IOAPIC");
    x2apic_controller_instance.initialize();
    pic_controller_instance.disable();
    ioapic_controller_instance.initialize();
    new_controller = &ioapic_controller_instance;
    controller_name = "IOAPIC (via x2APIC)";
  } else if (CPU::the().has_apic() && m_has_memory_manager) {
    fk::algorithms::klog("HW_INTERRUPT", "Enabling APIC + IOAPIC");
    apic_controller_instance.initialize();
    pic_controller_instance.disable();
    ioapic_controller_instance.initialize();
    new_controller = &ioapic_controller_instance;
    controller_name = "IOAPIC (via APIC)";
  } else {
    fk::algorithms::klog("HW_INTERRUPT", "Enabling PIC8259");
    pic_controller_instance.initialize();
    new_controller = &pic_controller_instance;
    controller_name = "PIC8259";
  }

  if (new_controller != m_controller) {
    set_controller(new_controller);
    fk::algorithms::klog("HW_INTERRUPT",
                         "Hardware interrupt controller set to: %s",
                         controller_name.c_str());
  }
}

void HardwareInterruptManager::set_controller(HardwareInterrupt *controller) {
  m_controller = controller;
}

void HardwareInterruptManager::initialize() {
  select_and_configure_controller();
}

void HardwareInterruptManager::set_memory_manager(bool is_memory_manager) {
  bool old_has_memory_manager = m_has_memory_manager;
  m_has_memory_manager = is_memory_manager;
  if (!old_has_memory_manager && m_has_memory_manager) {
    fk::algorithms::klog(
        "HW_INTERRUPT",
        "Memory manager enabled, re-evaluating interrupt controller.");
    select_and_configure_controller();
  }
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
