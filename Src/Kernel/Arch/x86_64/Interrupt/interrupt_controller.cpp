#include <Kernel/Arch/x86_64/Interrupt/HardwareInterrupts/hardware_interrupt_manager.h>
#include <Kernel/Arch/x86_64/Interrupt/HardwareInterrupts/InterruptController/8259_pic.h>
#include <Kernel/Arch/x86_64/Interrupt/interrupt_controller.h>
#include <Kernel/Arch/x86_64/Interrupt/interrupt_types.h>
#include <Kernel/Arch/x86_64/Interrupt/isr_stubs.h>
#include <Kernel/Arch/x86_64/Interrupt/non_maskable_interrupt.h>

#include <Kernel/Arch/x86_64/Interrupt/Handler/handlers.h>
#include <Kernel/Arch/x86_64/Interrupt/HardwareInterrupts/timer_interrupt.h>
#include <Kernel/Arch/x86_64/Segments/gdt.h>

#include <Kernel/Clock/clock_interrupt.h>

#include <LibFK/Algorithms/log.h>

extern "C" void flush_idt(void *idtr);

void InterruptController::initialize() {
  if (m_initialized) {
    fk::algorithms::kwarn("INTERRUPT", "IDT already initialized, skipping");
    return;
  }
  fk::algorithms::klog("INTERRUPT", "Initializing IDT and Hardware Interrupts...");
  disable_interrupt();

  clear();

  for (size_t i = 0; i < MAX_x86_64_IDT_SIZE; ++i) {
    set_gate(i, g_isr_stubs[i], GateType::InterruptGate, 0x08, 0);
    register_interrupt(default_handler, i);
  }

  // IST assignments per Intel SDM Vol.3 §6.14.1 / AMD APM Vol.2 §8.6:
  // IST1: Double Fault (#8) — prevents triple fault on stack corruption
  // IST2: NMI (#2) — prevents triple fault if NMI fires on corrupted stack
  // IST3: Machine Check (#18) — must not nest on a potentially-damaged stack
  set_gate(2,  g_isr_stubs[2],  GateType::InterruptGate, 0x08, 2);
  set_gate(8,  g_isr_stubs[8],  GateType::InterruptGate, 0x08, 1);
  set_gate(18, g_isr_stubs[18], GateType::InterruptGate, 0x08, 3);

  // NOTE: Interrupt Exception
  register_interrupt(divide_by_zero_handler, 0);
  register_interrupt(debug_handler, 1);
  register_interrupt(nmi_handler, 2);
  register_interrupt(breakpoint_handler, 3);
  register_interrupt(overflow_handler, 4);
  register_interrupt(bound_range_exceeded_handler, 5);
  register_interrupt(invalid_opcode_handler, 6);
  register_interrupt(device_not_available_handler, 7);
  register_interrupt(double_fault_handler, 8);
  register_interrupt(invalid_tss_handler, 10);
  register_interrupt(segment_not_present_handler, 11);
  register_interrupt(stack_segment_fault_handler, 12);
  register_interrupt(general_protection_handler, 13);
  register_interrupt(page_fault_handler, 14);
  register_interrupt(x87_fpu_floating_point_error_handler, 16);
  register_interrupt(alignment_check_handler, 17);
  register_interrupt(machine_check_handler, 18);
  register_interrupt(simd_floating_point_exception_handler, 19);
  register_interrupt(virtualization_exception_handler, 20);

  // NOTE: Interrupt Routine
  register_interrupt(timer_handler, 32);         // IRQ0 -> timer
  register_interrupt(keyboard_handler, 33);      // IRQ1 -> keyboard
  register_interrupt(mouse_handler, 44);         // IRQ12 -> PS/2 Mouse
  register_interrupt(clock_handler, 40);         // IRQ8 -> Clock;
  register_interrupt(ata_primary_handler, 46);   // IRQ14 -> primary ATA
  register_interrupt(ata_secondary_handler, 47); // IRQ15 -> secondary ATA

  // NOTE: Load IDT
  load();

  // Initialize the TimerManager
  TimerManager::the().initialize(1000);

  NMI::enable_nmi();

  HardwareInterruptManager::the().initialize();
  ClockManager::the().initialize();

  m_initialized = true;
  fk::algorithms::klog("INTERRUPT",
                       "Interrupt descriptor table initialized using %s",
                       HardwareInterruptManager::the().get_name().c_str());
}

void InterruptController::clear() {
  /*TODO: Apply this log when we work with LogLevel
  fk::algorithms::kdebug("INTERRUPT", "Clearing all IDT entries and handlers");
  */
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

  /*TODO: Apply this log when we work with LogLevel
  fk::algorithms::kdebug(
      "IDT", "Gate set: vector=%u handler=%p type=%u selector=%x ist=%u",
      vector, new_interrupt, type, selector, ist);
  */
}

void InterruptController::register_interrupt(interrupt new_interrupt,
                                             uint8_t vector) {
  /*TODO: Apply this log when we work with LogLevel
                                              fk::algorithms::kdebug("INTERRUPT",
                         "Registered handler: vector=%u, handler=%p", vector,
                         new_interrupt);
  */
  m_handlers[vector] = new_interrupt;
}

void InterruptController::load() {
  idt_ptr ptr;
  ptr.limit = static_cast<uint16_t>(sizeof(m_entries) - 1);
  ptr.base = reinterpret_cast<uint64_t>(&m_entries);
  flush_idt(&ptr);
  /*TODO: Apply this log when we work with LogLevel
  fk::algorithms::kdebug("INTERRUPT", "IDT loaded to CPU", ptr.base, ptr.limit);
  */
}

interrupt InterruptController::get_interrupt(uint8_t vector) {
  return m_handlers[vector];
}
