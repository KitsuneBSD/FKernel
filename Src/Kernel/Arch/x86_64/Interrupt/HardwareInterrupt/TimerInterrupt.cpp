#include <Kernel/Arch/x86_64/Interrupt/HardwareInterrupts/InterruptController/apic.h>
#include <Kernel/Arch/x86_64/Interrupt/HardwareInterrupts/TimerController/apic_timer.h>
#include <Kernel/Arch/x86_64/Interrupt/HardwareInterrupts/TimerController/hpet.h>
#include <Kernel/Arch/x86_64/Interrupt/HardwareInterrupts/TimerController/pit.h>
#include <Kernel/Arch/x86_64/Interrupt/HardwareInterrupts/TimerInterrupt.h>
#include <Kernel/Hardware/cpu.h>

void TimerManager::initialize(uint32_t freq) {
  if (CPU::the().has_hpet() && m_has_memory_manager) {
    fk::algorithms::klog("TIMER", "HPET timer selected");
    static HPETTimer hpet_timer;
    m_timer = &hpet_timer;
  } else if (CPU::the().has_apic() && m_has_memory_manager) {
    fk::algorithms::klog("TIMER", "APIC timer selected");
    static APICTimer apic_timer;
    m_timer = &apic_timer;
  } else {
    fk::algorithms::klog("TIMER", "PIT timer selected");
    static PITTimer pit;
    m_timer = &pit;
  }
  m_timer->initialize(freq);
}

void TimerManager::sleep(uint64_t ticks) {
  if (m_timer) {
    fk::algorithms::kdebug("TIMER", "Sleeping for %lu ticks", ticks);
    m_tick.the().sleep(ticks);
  } else {
    fk::algorithms::kwarn("TIMER", "No timer available to sleep!");
  }
}
