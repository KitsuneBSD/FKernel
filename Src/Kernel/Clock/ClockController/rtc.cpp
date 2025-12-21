#include <Kernel/Clock/ClockController/rtc.h>
#include <Kernel/Arch/x86_64/io.h>
#include <LibFK/Utilities/converter.h>

uint64_t RTCClock::s_ticks = 0;

uint8_t RTCClock::read_register(uint8_t reg) {
  outb(RTC_ADDRESS_PORT, (inb(RTC_ADDRESS_PORT) & 0x80) | reg);
  return inb(RTC_DATA_PORT);
}

void RTCClock::write_register(uint8_t reg, uint8_t value) {
  outb(RTC_ADDRESS_PORT, (inb(RTC_ADDRESS_PORT) & 0x80) | reg);
  outb(RTC_DATA_PORT, value);
}

fk::core::Result<void> RTCClock::initialize(uint32_t frequency) {
  m_frequency = frequency;

  asm volatile("cli");

  uint8_t prev_b = read_register(RTC_REG_B);
  write_register(RTC_REG_B, prev_b | 0x40); // Enable periodic IRQ

  set_frequency(frequency);

  asm volatile("sti");

  fk::algorithms::klog("RTC", "RTC timer initialized at ~%u Hz", m_frequency);
  return fk::core::Result<void>();
}

void RTCClock::set_frequency(uint32_t frequency) {
  if (frequency == 0)
    return;

  uint8_t rate = 15;
  if (frequency >= 8192)
    rate = 3;
  else if (frequency >= 4096)
    rate = 4;
  else if (frequency >= 2048)
    rate = 5;
  else if (frequency >= 1024)
    rate = 6;
  else if (frequency >= 512)
    rate = 7;
  else if (frequency >= 256)
    rate = 8;
  else if (frequency >= 128)
    rate = 9;
  else if (frequency >= 64)
    rate = 10;
  else if (frequency >= 32)
    rate = 11;
  else if (frequency >= 16)
    rate = 12;
  else if (frequency >= 8)
    rate = 13;
  else if (frequency >= 4)
    rate = 14;

  m_frequency = 32768 >> (rate - 1);

  asm volatile("cli");

  uint8_t prev_a = read_register(RTC_REG_A);
  write_register(RTC_REG_A, (prev_a & 0xF0) | rate);

  asm volatile("sti");
}

DateTime RTCClock::datetime() {
  DateTime dt;
  uint8_t register_b = read_register(RTC_REG_B);

  // Read RTC registers
  uint8_t second = read_register(0x00);
  uint8_t minute = read_register(0x02);
  uint8_t hour = read_register(0x04);
  uint8_t day = read_register(0x07);
  uint8_t month = read_register(0x08);
  uint8_t year = read_register(0x09);

  // Convert BCD to binary if necessary
  if (!(register_b &
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
  dt.year = year + 2000; // Assuming year is 2-digit and in 21st century. A more
                         // robust solution would involve reading the century
                         // from CMOS register 0x32 if available.

  return dt;
}
