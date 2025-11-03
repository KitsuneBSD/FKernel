#pragma once

#include <Kernel/Arch/x86_64/io.h>
#include <LibFK/Types/types.h>

/**
 * @brief Global tick counter incremented by the PIT interrupt
 */
static inline uint64_t ticks = 0;

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
class PIT {
private:
  PIT() = default; ///< Private constructor for singleton

  /**
   * @brief Set the PIT frequency
   *
   * @param frequency Frequency in Hz
   */
  void set_frequency(uint32_t frequency);

public:
  /**
   * @brief Get the singleton instance of the PIT
   *
   * @return Reference to the PIT instance
   */
  static PIT &the() {
    static PIT inst;
    return inst;
  }

  /**
   * @brief Initialize the PIT with a given frequency
   *
   * @param frequency Frequency in Hz
   */
  void initialize(uint32_t frequency);

  /**
   * @brief Sleep for a specified number of milliseconds
   *
   * This method uses the PIT tick counter to provide a simple blocking delay.
   *
   * @param ms Number of milliseconds to sleep
   */
  void sleep(uint64_t ms);

  /**
   * @brief Get the current tick count
   *
   * @return Number of PIT ticks since initialization
   */
  uint64_t get_ticks() const { return ticks; }
};
