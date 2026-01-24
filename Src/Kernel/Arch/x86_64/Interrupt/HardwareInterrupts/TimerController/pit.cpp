#include <Kernel/Arch/x86_64/Interrupt/HardwareInterrupts/TimerController/pit.h>
#include <Kernel/Arch/x86_64/io.h>

void PITTimer::initialize(uint32_t frequency) {
  m_frequency = frequency;
  set_frequency(frequency);
}

void PITTimer::set_frequency(uint32_t frequency) {
  uint16_t divisor = 1193180 / frequency;
  outb(PIT_COMMAND, PIT_CMD_RATE_GEN);
  outb(PIT_CHANNEL0, (uint8_t)(divisor & 0xFF));
  outb(PIT_CHANNEL0, (uint8_t)((divisor >> 8) & 0xFF));
}
