#include <Kernel/Arch/x86_64/Interrupt/HardwareInterrupts/pit.h>
#include <LibFK/log.h>

void PIT::set_frequency(uint32_t frequency) {
  uint16_t divisor = 1193182 / frequency;
  outb(PIT_COMMAND, PIT_CMD_RATE_GEN);
  outb(PIT_CHANNEL0, static_cast<uint8_t>(divisor & 0xFF));        // LSB
  outb(PIT_CHANNEL0, static_cast<uint8_t>((divisor >> 8) & 0xFF)); // MSB
}

void PIT::initialize(uint32_t frequency) {
  set_frequency(frequency);
  klog("PIT", "PIT initialized at %u Hz", frequency);
}
