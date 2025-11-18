#pragma once

#include <Kernel/Arch/x86_64/Interrupt/HardwareInterrupts/ClockInterrupt.h>
#include <Kernel/Arch/x86_64/io.h>
#include <LibFK/Algorithms/log.h>
#include <LibFK/Core/Result.h>
#include <LibFK/Types/types.h>

/**
 * @brief CMOS Address Port
 */
constexpr uint16_t CMOS_ADDRESS_PORT = 0x70;

/**
 * @brief CMOS Data Port
 */
constexpr uint16_t CMOS_DATA_PORT = 0x71;

/**
 * @brief Represents the CMOS Clock as a fallback for RTC.
 *
 * This class provides a basic implementation of the Clock interface
 * to read date and time from the CMOS when RTC is not available or fails.
 */
class CMOSClock : public Clock {
private:
  fk::text::String name = "CMOS";

  uint8_t read_register(uint8_t reg);
public:
  fk::core::Result<void> initialize(uint32_t frequency) override;
  fk::text::String get_name() override { return name; }
  DateTime datetime() override;
};
