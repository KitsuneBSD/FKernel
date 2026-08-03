#include <Kernel/Arch/x86_64/Interrupt/HardwareInterrupts/TimerController/pit.h>
#include <Kernel/Arch/x86_64/io.h>
#include <LibFK/Algorithms/Logging/log.h>

void PITTimer::initialize(uint32_t frequency) {
  fk::algorithms::klog("PIT", "Initializing PIT at %u Hz", frequency);
  m_frequency = frequency;
  set_frequency(frequency);
}

void PITTimer::set_frequency(uint32_t frequency) {
  uint16_t divisor = 1193180 / frequency;
  outb(PIT_COMMAND, PIT_CMD_RATE_GEN);
  outb(PIT_CHANNEL0, (uint8_t)(divisor & 0xFF));
  outb(PIT_CHANNEL0, (uint8_t)((divisor >> 8) & 0xFF));
}

void PITTimer::disable() {
  // Switch channel 0 to one-shot mode with initial count 0 — stops periodic IRQ0.
  outb(PIT_COMMAND, 0x30); // channel 0, lo/hi, one-shot (mode 0)
  outb(PIT_CHANNEL0, 0);
  outb(PIT_CHANNEL0, 0);
  fk::algorithms::klog("PIT", "PIT channel 0 disabled (one-shot, count=0)");
}

// PIT channel 2 ports (speaker gate — safe for polling without IRQ)
static constexpr uint16_t PIT_CHANNEL2 = 0x42;
static constexpr uint16_t PIT_KBC_GATE = 0x61;

void PITTimer::pit_wait_ms(uint32_t ms) {
  // PIT base frequency: 1193182 Hz. Each ms = 1193 ticks (rounded).
  constexpr uint32_t PIT_HZ = 1193182;
  constexpr uint32_t TICKS_PER_MS = PIT_HZ / 1000;

  uint32_t total_ticks = ms * TICKS_PER_MS;
  // Channel 2 one-shot, lo/hi byte access, mode 0 (terminal count).
  outb(PIT_COMMAND, 0xB0); // channel 2, lo/hi, mode 0
  outb(PIT_CHANNEL2, (uint8_t)(total_ticks & 0xFF));
  outb(PIT_CHANNEL2, (uint8_t)((total_ticks >> 8) & 0xFF));

  // Enable channel 2 gate (bit 0 of port 0x61), clear speaker output (bit 1).
  uint8_t gate = inb(PIT_KBC_GATE);
  outb(PIT_KBC_GATE, (uint8_t)((gate & ~0x02) | 0x01));

  // Poll output status (bit 5 of port 0x61) until high — count reached zero.
  while (!(inb(PIT_KBC_GATE) & 0x20))
    asm volatile("pause");

  // Restore gate state.
  outb(PIT_KBC_GATE, gate);
}
