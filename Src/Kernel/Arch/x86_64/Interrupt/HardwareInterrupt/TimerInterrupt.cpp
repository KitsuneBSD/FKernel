#include <Kernel/Arch/x86_64/Interrupt/HardwareInterrupts/InterruptController/apic.h>
#include <Kernel/Arch/x86_64/Interrupt/HardwareInterrupts/TimerController/apic_timer.h>
#include <Kernel/Arch/x86_64/Interrupt/HardwareInterrupts/TimerController/pit.h>
#include <Kernel/Arch/x86_64/Interrupt/HardwareInterrupts/TimerInterrupt.h>
#include <Kernel/Hardware/Cpu.h>

void TimerManager::initialize(uint32_t freq) {
  if (CPU::the().has_apic() && m_has_memory_manager) {
    klog("TIMER", "APIC timer selected");
    static APICTimer apic_timer;
    m_timer = &apic_timer;
  } else {
    klog("TIMER", "PIT timer selected");
    static PITTimer pit;
    m_timer = &pit;
  }
  m_timer->initialize(freq);
}

void TimerManager::sleep(uint64_t ms) {
  if (m_timer) {
    kdebug("TIMER", "Sleeping for %lu ms", ms);
    m_timer->sleep(ms);
  } else {
    kwarn("TIMER", "No timer available to sleep!");
  }
}

void TimerManager::increment_ticks() {
  if (m_timer)
    m_timer->increment_ticks();
}
