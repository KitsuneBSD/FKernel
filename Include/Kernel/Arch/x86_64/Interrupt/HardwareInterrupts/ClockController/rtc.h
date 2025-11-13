#pragma once

#include <Kernel/Arch/x86_64/io.h>
#include <LibFK/Algorithms/log.h>
#include <LibFK/Types/types.h>

/**
 * @brief RTC Address Port
 */
constexpr uint16_t RTC_ADDRESS_PORT = 0x70;

/**
 * @brief RTC Data Port
 */
constexpr uint16_t RTC_DATA_PORT = 0x71;

constexpr uint8_t RTC_REG_A = 0x0A;
constexpr uint8_t RTC_REG_B = 0x0B;

/**
 * @brief Represents the Real-Time Clock (RTC)
 *
 * This class allows initializing the RTC as a timer, setting its frequency,
 * tracking system ticks, and providing simple delay functionality via sleep.
 */
class RTCClock {
private:
  uint32_t m_frequency = 0;

  uint8_t read_register(uint8_t reg);
  void write_register(uint8_t reg, uint8_t value);

public:
  static RTCClock &the() {
    static RTCClock clock;
    return clock;
  }

  void initialize(uint32_t frequency);
  void set_frequency(uint32_t frequency);
};
