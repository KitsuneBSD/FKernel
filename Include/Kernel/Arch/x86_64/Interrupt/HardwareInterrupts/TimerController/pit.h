#pragma once

#include <Kernel/Arch/x86_64/Interrupt/HardwareInterrupts/TimerInterrupt.h>
#include <Kernel/Arch/x86_64/io.h>
#include <LibFK/Algorithms/log.h>
#include <LibFK/Types/types.h>

/**
 * @brief PIT Channel 0 data port
 */
constexpr uint16_t PIT_CHANNEL0 = 0x40;

/**
 * @brief PIT Command port
 */
constexpr uint16_t PIT_COMMAND = 0x43;

/**
 * @brief Command byte to configure the PIT in rate generator mode
 */
constexpr uint8_t PIT_CMD_RATE_GEN = 0x34;

/**
 * @brief Represents the Programmable Interval Timer (PIT)
 *
 * This singleton class allows initializing the PIT, setting its frequency,
 * tracking system ticks, and providing simple delay functionality via sleep.
 */

// PIT Timer implementation
class PITTimer : public Timer {
private:
  uint64_t m_ticks = 0;
  uint32_t m_frequency = 0;

public:
  void initialize(uint32_t frequency) override;
  void set_frequency(uint32_t frequency);
  void increment_ticks() override { m_ticks++; }
  uint64_t get_ticks() override { return m_ticks; }
  void sleep(uint64_t ms) override;
};
