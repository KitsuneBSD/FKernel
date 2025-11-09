#pragma once

#include <Kernel/Arch/x86_64/Interrupt/HardwareInterrupts/TimerInterrupt.h>
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

/**
 * @brief Represents the Real-Time Clock (RTC)
 *
 * This class allows initializing the RTC as a timer, setting its frequency,
 * tracking system ticks, and providing simple delay functionality via sleep.
 */
class RTCTimer : public Timer {
private:
    uint64_t m_ticks = 0;
    uint32_t m_frequency = 0;

    uint8_t read_register(uint8_t reg);
    void write_register(uint8_t reg, uint8_t value);

public:
    void initialize(uint32_t frequency) override;
    void set_frequency(uint32_t frequency);
    void increment_ticks() override { m_ticks++; }
    uint64_t get_ticks() override { return m_ticks; }
    void sleep(uint64_t ms) override;
};
