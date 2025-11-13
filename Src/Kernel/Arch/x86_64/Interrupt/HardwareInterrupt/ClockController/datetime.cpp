#include <Kernel/Arch/x86_64/Interrupt/HardwareInterrupts/ClockController/rtc.h>
#include <Kernel/Arch/x86_64/Interrupt/HardwareInterrupts/ClockInterrupt.h>
#include <LibFK/Utilities/converter.h>

static inline uint8_t read_cmos(uint8_t reg) {
  outb(RTC_ADDRESS_PORT, (inb(RTC_ADDRESS_PORT) & 0x80) | reg);
  return inb(RTC_DATA_PORT);
}

DateTime DateTime::now() {
  DateTime dt{};
  uint8_t reg_b;
  uint8_t last_second, last_minute, last_hour, last_day, last_month, last_year;

  do {
    last_second = read_cmos(0x00);
    last_minute = read_cmos(0x02);
    last_hour = read_cmos(0x04);
    last_day = read_cmos(0x07);
    last_month = read_cmos(0x08);
    last_year = read_cmos(0x09);
  } while (last_second != read_cmos(0x00));

  reg_b = read_cmos(RTC_REG_B);

  if (!(reg_b & 0x04)) {
    dt.second = bcd_to_bin(last_second);
    dt.minute = bcd_to_bin(last_minute);
    dt.hour = bcd_to_bin(last_hour & 0x7F);
    dt.day = bcd_to_bin(last_day);
    dt.month = bcd_to_bin(last_month);
    dt.year = bcd_to_bin(last_year);
  } else {
    dt.second = last_second;
    dt.minute = last_minute;
    dt.hour = last_hour & 0x7F;
    dt.day = last_day;
    dt.month = last_month;
    dt.year = last_year;
  }

  dt.year += 2000;

  if (!(reg_b & 0x02) && (dt.hour & 0x80)) {
    dt.hour = ((dt.hour & 0x7F) + 12) % 24;
  }

  return dt;
}

String DateTime::to_string() const {
  char buffer[32];
  snprintf(buffer, sizeof(buffer), "%02u/%02u/%04u %02u:%02u:%02u", day, month,
           year, hour, minute, second);
  return String(buffer);
}

void DateTime::print() {
  klog("CLOCK MANAGER", "%02u/%02u/%04u %02u:%02u:%02u", day, month, year, hour,
       minute, second);
}
