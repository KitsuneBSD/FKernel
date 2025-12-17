#pragma once

#include <LibFK/Algorithms/log.h>
#include <LibFK/Text/string.h>
#include <LibFK/Types/types.h>

class HardwareInterrupt {
public:
  virtual void initialize() = 0;
  virtual void mask_interrupt(uint8_t interrupt_number) = 0;
  virtual void unmask_interrupt(uint8_t interrupt_number) = 0;
  virtual void send_eoi(uint8_t interrupt_number) = 0;
  virtual fk::text::String get_name() = 0;
  virtual ~HardwareInterrupt() = default;
};

class HardwareInterruptManager {
friend class InterruptController;
private:
  HardwareInterrupt *m_controller = nullptr;
  bool m_has_memory_manager = false;

  HardwareInterruptManager() = default;

public:
  static HardwareInterruptManager &the() {
    static HardwareInterruptManager inst;
    return inst;
  }

  /**
   * @brief Selects the best available hardware interrupt m_controller
   *
   * Checks for APIC support; if present, enables APIC + IOAPIC.
   * Otherwise, defaults to PIC8259.
   */
  void initialize();
  void mask_interrupt(uint8_t irq);
  void unmask_interrupt(uint8_t irq);
  void send_eoi(uint8_t irq);

  void set_controller(HardwareInterrupt *controller);
  void set_memory_manager(bool is_memory_manager);

protected:
  void select_and_configure_controller();

  fk::text::String get_name() { return m_controller ? m_controller->get_name() : "None"; }
  HardwareInterrupt *get_m_controller() { return m_controller; }
};
