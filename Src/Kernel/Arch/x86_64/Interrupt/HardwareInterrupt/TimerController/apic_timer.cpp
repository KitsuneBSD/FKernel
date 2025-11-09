#include "Kernel/Arch/x86_64/Interrupt/HardwareInterrupts/InterruptController/apic.h"
#include <Kernel/Arch/x86_64/Interrupt/HardwareInterrupts/HardwareInterrupt.h>
#include <Kernel/Arch/x86_64/Interrupt/HardwareInterrupts/TimerController/apic_timer.h>

void APICTimer::initialize(uint32_t frequency) {
  m_ticks = 0;

  APIC::the().calibrate_timer();      // Descobre ticks/ms
  APIC::the().setup_timer(frequency); // Configura periodicidade

  klog("TIMER", "Initializing APIC Timer at %u Hz", frequency);
}

void APICTimer::sleep(uint64_t ms) {
  uint64_t start = m_ticks;
  uint64_t target_ticks = start + (ms * 100 / 1000);
  while (m_ticks < target_ticks) {
    asm volatile("hlt");
  }
}
