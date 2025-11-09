#pragma once

#include <Kernel/Arch/x86_64/Interrupt/HardwareInterrupts/HardwareInterrupt.h>
#include <Kernel/Arch/x86_64/Interrupt/HardwareInterrupts/TimerInterrupt.h>
#include <Kernel/Arch/x86_64/Interrupt/interrupt_controller.h>
#include <LibFK/Algorithms/log.h>
#include <LibFK/Types/types.h>

/**
 * @brief APIC Timer implementation
 *
 * Uses APIC LVT Timer as a hardware interrupt. Ticks are counted
 * in the interrupt handler.
 */
class APICTimer : public Timer {
private:
  uint64_t m_ticks = 0;

public:
  void initialize(uint32_t frequency) override;
  uint64_t get_ticks() override { return m_ticks; }
  void increment_ticks() override { m_ticks++; }

  void sleep(uint64_t ms) override;
};
