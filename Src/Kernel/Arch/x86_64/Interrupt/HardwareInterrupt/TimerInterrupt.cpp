#include <Kernel/Arch/x86_64/Interrupt/HardwareInterrupts/InterruptController/apic.h>
#include <Kernel/Arch/x86_64/Interrupt/HardwareInterrupts/TimerController/apic_timer.h>
#include <Kernel/Arch/x86_64/Interrupt/HardwareInterrupts/TimerController/hpet.h>
#include <Kernel/Arch/x86_64/Interrupt/HardwareInterrupts/TimerController/pit.h>
#include <Kernel/Arch/x86_64/Interrupt/HardwareInterrupts/TimerInterrupt.h>
#include <Kernel/Hardware/cpu.h>

void TimerManager::initialize(uint32_t freq) {
  select_and_configure_timer(freq);
}

void TimerManager::select_and_configure_timer(uint32_t freq) {
  static HPETTimer hpet_timer_instance;
  static APICTimer apic_timer_instance;
  static PITTimer pit_timer_instance;

  Timer *new_timer = nullptr;
  fk::text::String timer_name = "None";

  if (CPU::the().has_hpet() && m_has_memory_manager) {
    fk::algorithms::klog("TIMER", "HPET timer selected");
    new_timer = &hpet_timer_instance;
    timer_name = "HPET";
  } else if (CPU::the().has_apic() && m_has_memory_manager) {
    fk::algorithms::klog("TIMER", "APIC timer selected");
    new_timer = &apic_timer_instance;
    timer_name = "APIC Timer";
  } else {
    fk::algorithms::klog("TIMER", "PIT timer selected");
    new_timer = &pit_timer_instance;
    timer_name = "PIT";
  }

  if (new_timer != m_timer) {
    set_timer(new_timer);
    TickManager::the().set_frequency(freq);
    if (m_timer) {
      m_timer->initialize(freq);
      fk::algorithms::klog("TIMER", "Timer controller set to: %s",
                           timer_name.c_str());
    }
  }
}

void TimerManager::set_timer(Timer *timer) { m_timer = timer; }

void TimerManager::sleep(uint64_t awaited_ticks) {
  if (m_timer) {
    // The TickManager is independent of the chosen Timer hardware, it just
    // counts increments. The actual hardware timer (PIT, APIC, HPET) is
    // responsible for calling increment_ticks. So, here we just use the
    // TickManager to wait for a certain number of ticks.
    m_tick.sleep(awaited_ticks);
  } else {
    fk::algorithms::kwarn("TIMER", "No timer available to sleep!");
  }
}

void TimerManager::set_memory_manager(bool has_memory_manager) {
  bool old_has_memory_manager = m_has_memory_manager;
  m_has_memory_manager = has_memory_manager;
  if (!old_has_memory_manager && m_has_memory_manager) {
    fk::algorithms::klog(
        "TIMER", "Memory manager enabled, re-evaluating timer controller.");
    // Re-select and configure the timer with the current frequency (assuming a
    // default or stored freq) For now, we'll use a placeholder frequency. A
    // better approach would be to store the frequency.
    select_and_configure_timer(100); // Re-initialize with a default frequency
    TickManager::the().set_frequency(
        100); // Update TickManager with the default frequency
  }
}
