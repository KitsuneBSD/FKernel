#pragma once

#include <Kernel/Arch/x86_64/Interrupt/HardwareInterrupts/HardwareInterrupt.h>
#include <LibFK/Container/string.h>
#include <LibFK/Types/types.h>

/**
 * @brief Intel 8259 Programmable Interrupt Controller (PIC)
 *
 */
class PIC8259 : public HardwareInterrupt {
private:
  static uint16_t get_irr();
  static uint16_t get_isr();

  String m_name = "8259PIC";

public:
  void initialize() override;
  void mask_interrupt(uint8_t irq) override;
  void unmask_interrupt(uint8_t irq) override;
  void send_eoi(uint8_t irq) override;
  String get_name() override { return m_name; }

  /**
   * @brief Disable 8259PIC
   */
  void disable();
};
