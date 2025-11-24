#include <Kernel/Arch/x86_64/Interrupt/HardwareInterrupts/HardwareInterrupt.h>
#include <Kernel/Arch/x86_64/Interrupt/HardwareInterrupts/InterruptController/8259_pic.h>
#include <Kernel/Arch/x86_64/Interrupt/interrupt_controller.h>
#include <Kernel/Arch/x86_64/Interrupt/interrupt_types.h>
#include <Kernel/Arch/x86_64/Interrupt/isr_stubs.h>
#include <Kernel/Arch/x86_64/Interrupt/non_maskable_interrupt.h>

#include <Kernel/Arch/x86_64/Interrupt/Handler/handlers.h>
#include <Kernel/Arch/x86_64/Interrupt/HardwareInterrupts/TimerInterrupt.h>

#include <Kernel/Arch/x86_64/Segments/gdt.h>

#include <LibFK/Algorithms/log.h>

extern "C" void flush_idt(void *idtr);

void InterruptController::initialize() {
  disable_interrupt();

  clear();

  HardwareInterruptManager::the().initialize();

  fk::algorithms::kdebug("INTERRUPT", "Setting up IDT entries...");
  for (size_t i = 0; i < MAX_x86_64_IDT_SIZE; ++i) {
    uint8_t multiple_ist_selector = (i % 7) + 1;
    if (i < 32) { // Exceptions
      set_gate(i, g_isr_stubs[i], GateType::InterruptGate, 0x08,
               *ist_stacks[multiple_ist_selector]);
    } else { // IRQs
      set_gate(i, g_isr_stubs[i], GateType::TrapGate, 0x08,
               *ist_stacks[multiple_ist_selector]);
    }
    register_interrupt(default_handler, i);
  }

  fk::algorithms::kdebug("INTERRUPT", "Registering divide_by_zero_handler for vector 0");
  register_interrupt(divide_by_zero_handler, 0);
  fk::algorithms::kdebug("INTERRUPT", "Registering debug_handler for vector 1");
  register_interrupt(debug_handler, 1);
  fk::algorithms::kdebug("INTERRUPT", "Registering nmi_handler for vector 2");
  register_interrupt(nmi_handler, 2);
  fk::algorithms::kdebug("INTERRUPT", "Registering breakpoint_handler for vector 3");
  register_interrupt(breakpoint_handler, 3);
  fk::algorithms::kdebug("INTERRUPT", "Registering overflow_handler for vector 4");
  register_interrupt(overflow_handler, 4);
  fk::algorithms::kdebug("INTERRUPT", "Registering bound_range_exceeded_handler for vector 5");
  register_interrupt(bound_range_exceeded_handler, 5);
  fk::algorithms::kdebug("INTERRUPT", "Registering invalid_opcode_handler for vector 6");
  register_interrupt(invalid_opcode_handler, 6);
  fk::algorithms::kdebug("INTERRUPT", "Registering device_not_available_handler for vector 7");
  register_interrupt(device_not_available_handler, 7);
  fk::algorithms::kdebug("INTERRUPT", "Registering double_fault_handler for vector 8");
  register_interrupt(double_fault_handler, 8);
  fk::algorithms::kdebug("INTERRUPT", "Registering invalid_tss_handler for vector 10");
  register_interrupt(invalid_tss_handler, 10);
  fk::algorithms::kdebug("INTERRUPT", "Registering segment_not_present_handler for vector 11");
  register_interrupt(segment_not_present_handler, 11);
  fk::algorithms::kdebug("INTERRUPT", "Registering stack_segment_fault_handler for vector 12");
  register_interrupt(stack_segment_fault_handler, 12);
  fk::algorithms::kdebug("INTERRUPT", "Registering general_protection_handler for vector 13");
  register_interrupt(general_protection_handler, 13);
  fk::algorithms::kdebug("INTERRUPT", "Registering page_fault_handler for vector 14");
  register_interrupt(page_fault_handler, 14);
  fk::algorithms::kdebug("INTERRUPT", "Registering x87_fpu_floating_point_error_handler for vector 16");
  register_interrupt(x87_fpu_floating_point_error_handler, 16);
  fk::algorithms::kdebug("INTERRUPT", "Registering alignment_check_handler for vector 17");
  register_interrupt(alignment_check_handler, 17);
  fk::algorithms::kdebug("INTERRUPT", "Registering machine_check_handler for vector 18");
  register_interrupt(machine_check_handler, 18);
  fk::algorithms::kdebug("INTERRUPT", "Registering simd_floating_point_exception_handler for vector 19");
  register_interrupt(simd_floating_point_exception_handler, 19);
  fk::algorithms::kdebug("INTERRUPT", "Registering virtualization_exception_handler for vector 20");
  register_interrupt(virtualization_exception_handler, 20);

  fk::algorithms::kdebug("INTERRUPT", "Registering timer_handler for vector 32 (IRQ0)");
  register_interrupt(timer_handler, 32);         // IRQ0 -> timer
  fk::algorithms::kdebug("INTERRUPT", "Registering keyboard_handler for vector 33 (IRQ1)");
  register_interrupt(keyboard_handler, 33);      // IRQ1 -> keyboard
  fk::algorithms::kdebug("INTERRUPT", "Registering clock_handler for vector 40 (IRQ8)");
  register_interrupt(clock_handler, 40);         // IRQ8 -> Clock;
  fk::algorithms::kdebug("INTERRUPT", "Registering ata_primary_handler for vector 46 (IRQ14)");
  register_interrupt(ata_primary_handler, 46);   // IRQ14 -> primary ATA
  fk::algorithms::kdebug("INTERRUPT", "Registering ata_secondary_handler for vector 47 (IRQ15)");
  register_interrupt(ata_secondary_handler, 47); // IRQ15 -> secondary ATA

  load();

  NMI::enable_nmi();

  TimerManager::the().initialize(100);

  HardwareInterruptManager::the().unmask_interrupt(0);  // Timer
  HardwareInterruptManager::the().unmask_interrupt(1);  // Keyboard
  HardwareInterruptManager::the().unmask_interrupt(8);  // Clock
  HardwareInterruptManager::the().unmask_interrupt(14); // Primary ATA
  HardwareInterruptManager::the().unmask_interrupt(15); // Secondary ATA

  enable_interrupt();
  fk::algorithms::klog("INTERRUPT", "Interrupt descriptor table initialized using PIC8259");
}

void InterruptController::clear() {
  fk::algorithms::kdebug("INTERRUPT", "Clearing all IDT entries and handlers");
  for (size_t i = 0; i < MAX_x86_64_IDT_SIZE; ++i) {
    m_entries[i] = {};
    m_handlers[i] = nullptr;
  }
}

void InterruptController::set_gate(uint8_t vector, void (*new_interrupt)(),
                                   GateType type, uint16_t selector,
                                   uint8_t ist) {
  const uint64_t handler = reinterpret_cast<uint64_t>(new_interrupt);
  idt_entry &d = m_entries[vector];
  d.offset_low = static_cast<uint16_t>(handler & 0xFFFFu);
  d.selector = selector;
  d.ist = static_cast<uint8_t>(ist & 0x7u);
  d.type_attr = static_cast<uint8_t>(type);
  d.offset_mid = static_cast<uint16_t>((handler >> 16) & 0xFFFFu);
  d.offset_high = static_cast<uint32_t>((handler >> 32) & 0xFFFFFFFFu);
  d.zero = 0;
}

void InterruptController::register_interrupt(interrupt new_interrupt,
                                             uint8_t vector) {
  m_handlers[vector] = new_interrupt;
}

void InterruptController::load() {
  idt_ptr ptr;
  ptr.limit = static_cast<uint16_t>(sizeof(m_entries) - 1);
  ptr.base = reinterpret_cast<uint64_t>(&m_entries);
  flush_idt(&ptr);

  fk::algorithms::kdebug("INTERRUPT", "IDT loaded to CPU", ptr.base, ptr.limit);
}

interrupt InterruptController::get_interrupt(uint8_t vector) {
  return m_handlers[vector];
}
