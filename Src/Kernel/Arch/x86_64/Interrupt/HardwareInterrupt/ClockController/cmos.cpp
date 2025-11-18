#include <Kernel/Arch/x86_64/Interrupt/HardwareInterrupts/ClockController/cmos.h>
#include <LibFK/Utilities/converter.h>

uint8_t CMOSClock::read_register(uint8_t reg) {
  outb(CMOS_ADDRESS_PORT, (inb(CMOS_ADDRESS_PORT) & 0x80) | reg);
  return inb(CMOS_DATA_PORT);
}

fk::core::Result<void> CMOSClock::initialize(uint32_t frequency) {
  // CMOS is usually always available, no specific initialization needed apart
  // from reading. The frequency parameter is not directly applicable to CMOS
  // as a timer, but we honor the interface.
  (void)frequency; // Suppress unused parameter warning

  fk::algorithms::klog("CMOS", "CMOS clock initialized.");
  return fk::core::Result<void>();
}

DateTime CMOSClock::datetime() {
  DateTime dt;
  uint8_t status_b;

  // Read status register B to check BCD mode
  status_b = read_register(0x0B);

  // Read CMOS registers directly
  uint8_t second = read_register(0x00);
  uint8_t minute = read_register(0x02);
  uint8_t hour = read_register(0x04);
  uint8_t day = read_register(0x07);
  uint8_t month = read_register(0x08);
  uint8_t year = read_register(0x09);

  // Convert BCD to binary if necessary
  if (!(status_b &
        0x04)) { // Check if BCD mode is enabled (bit 2 of Register B)
    second = fk::utilities::bcd_to_bin(second);
    minute = fk::utilities::bcd_to_bin(minute);
    hour = fk::utilities::bcd_to_bin(hour);
    day = fk::utilities::bcd_to_bin(day);
    month = fk::utilities::bcd_to_bin(month);
    year = fk::utilities::bcd_to_bin(year);
  }

  dt.second = second;
  dt.minute = minute;
  dt.hour = hour;
  dt.day = day;
  dt.month = month;
  dt.year =
      year + 2000; // Simplified for 21st century. Improve later if needed.

  return dt;
}
