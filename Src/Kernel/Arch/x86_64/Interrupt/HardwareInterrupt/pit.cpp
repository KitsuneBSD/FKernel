#include <Kernel/Arch/x86_64/Interrupt/HardwareInterrupts/pit.h>
#include <LibFK/Algorithms/log.h>

void PIT::set_frequency(uint32_t frequency) {
  uint16_t divisor = 1193182 / frequency;
  outb(PIT_COMMAND, PIT_CMD_RATE_GEN);
  outb(PIT_CHANNEL0, static_cast<uint8_t>(divisor & 0xFF));        // LSB
  outb(PIT_CHANNEL0, static_cast<uint8_t>((divisor >> 8) & 0xFF)); // MSB
  kdebug("PIT", "Set frequency: %u Hz (divisor=%u)", frequency, divisor);
}

void PIT::initialize(uint32_t frequency) {
  set_frequency(frequency);
  klog("PIT", "PIT initialized at %u Hz", frequency);
}

void PIT::sleep(uint64_t ms) {
  const uint32_t pit_freq = 1193182;       // Hz base
  const uint32_t divisor = pit_freq / 100; // assume 100 Hz
  const uint64_t ticks_to_wait = (ms * pit_freq) / (divisor * 1000);

  outb(PIT_COMMAND, 0x00); // latch current count
  uint64_t start_count = inb(PIT_CHANNEL0);
  start_count |= (inb(PIT_CHANNEL0) << 8);

  uint64_t elapsed_ticks = 0;
  kdebug("PIT", "Starting sleep: %llu ms, waiting for %llu ticks", ms,
         ticks_to_wait);

  while (elapsed_ticks < ticks_to_wait) {
    outb(PIT_COMMAND, 0x00);
    uint64_t current_count = inb(PIT_CHANNEL0);
    current_count |= (inb(PIT_CHANNEL0) << 8);

    if (current_count <= start_count)
      elapsed_ticks += (start_count - current_count);
    else
      elapsed_ticks += (start_count + (divisor - current_count));

    start_count = current_count;
  }

  kdebug("PIT", "Sleep finished, elapsed ticks: %llu", elapsed_ticks);
}
