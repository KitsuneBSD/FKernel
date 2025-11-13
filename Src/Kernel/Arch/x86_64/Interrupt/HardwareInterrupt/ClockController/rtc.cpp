#include "Kernel/Arch/x86_64/Interrupt/interrupt_controller.h"
#include <Kernel/Arch/x86_64/Interrupt/HardwareInterrupts/ClockController/rtc.h>
#include <Kernel/Arch/x86_64/io.h>

uint8_t RTCClock::read_register(uint8_t reg) {
  outb(RTC_ADDRESS_PORT, (inb(RTC_ADDRESS_PORT) & 0x80) | reg);
  return inb(RTC_DATA_PORT);
}

void RTCClock::write_register(uint8_t reg, uint8_t value) {
  outb(RTC_ADDRESS_PORT, (inb(RTC_ADDRESS_PORT) & 0x80) | reg);
  outb(RTC_DATA_PORT, value);
}

void RTCClock::initialize(uint32_t frequency) {
  m_frequency = frequency;

  asm volatile("cli");

  uint8_t prev_b = read_register(RTC_REG_B);
  write_register(RTC_REG_B, prev_b | 0x40); // Enable periodic IRQ

  set_frequency(frequency);

  asm volatile("sti");

  klog("RTC", "RTC timer initialized at ~%u Hz", m_frequency);
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
