#pragma once

#include <Kernel/Arch/x86_64/Interrupt/HardwareInterrupts/TimerInterrupt.h>
#include <LibFK/Types/types.h>

constexpr uint64_t GENERAL_CAPABILITIES_ID = 0x00;
constexpr uint64_t GENERAL_CONFIGURATION = 0x10;
constexpr uint64_t MAIN_COUNTER_VALUE = 0xF0;
constexpr uint64_t TIMER0_CONFIGURATION = 0x100;
constexpr uint64_t TIMER0_COMPARATOR = 0x108;

class HPETTimer : public Timer {
private:
  uint32_t m_frequency = 0;
  volatile uint64_t *m_hpet_regs = nullptr;
  uint64_t m_counter_period = 0; // in femtoseconds (10^-15)

  uint64_t read_reg(uint64_t reg);
  void write_reg(uint64_t reg, uint64_t value);

public:
  void initialize(uint32_t frequency) override;
};
