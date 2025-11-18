#include "Kernel/Arch/x86_64/Interrupt/HardwareInterrupts/InterruptController/apic.h"
#include <Kernel/Arch/x86_64/Interrupt/HardwareInterrupts/HardwareInterrupt.h>
#include <Kernel/Arch/x86_64/Interrupt/HardwareInterrupts/TimerController/apic_timer.h>

void APICTimer::initialize(uint32_t frequency) {
  APIC::the().calibrate_timer();      // Descobre ticks/ms
  APIC::the().setup_timer(frequency); // Configura periodicidade

  fk::algorithms::klog("TIMER", "Initializing APIC Timer at %u Hz", frequency);
}
