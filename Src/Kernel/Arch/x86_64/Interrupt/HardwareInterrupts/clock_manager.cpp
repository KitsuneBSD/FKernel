#include <Kernel/Arch/x86_64/Interrupt/HardwareInterrupts/ClockController/cmos.h>
#include <Kernel/Arch/x86_64/Interrupt/HardwareInterrupts/ClockController/rtc.h>
#include <Kernel/Arch/x86_64/Interrupt/HardwareInterrupts/clock_interrupt.h>
#include <LibFK/Algorithms/log.h>

void ClockManager::initialize() {
  select_and_configure_clock();
  fk::algorithms::klog("CLOCK MANAGER", "Clock manager initialized.");
}

void ClockManager::select_and_configure_clock() {
  static RTCClock rtc_clock_instance;
  static CMOSClock cmos_clock_instance;

  Clock *new_clock = nullptr;
  fk::text::String clock_name = "None";

  if (rtc_clock_instance.initialize(1024).is_ok()) {
    new_clock = &rtc_clock_instance;
    clock_name = "RTC";
  } else {
    fk::algorithms::klog("CLOCK MANAGER", "RTC initialization failed, falling back to CMOS.");
    if (cmos_clock_instance.initialize(0).is_ok()) {
      new_clock = &cmos_clock_instance;
      clock_name = "CMOS";
    } else {
      fk::algorithms::klog("CLOCK MANAGER", "CMOS initialization also failed. No clock set.");
    }
  }

  if (new_clock != m_clock) {
    set_clock(new_clock);
    fk::algorithms::klog("CLOCK MANAGER", "Clock controller set to: %s", clock_name.c_str());
  } else {
    fk::algorithms::kdebug("CLOCK MANAGER", "Clock controller remains: %s", clock_name.c_str());
  }
}

void ClockManager::set_clock(Clock *clock) {
  m_clock = clock;
  if (m_clock) {
    fk::algorithms::klog("CLOCK MANAGER", "Set a new clock called %s",
                         m_clock->get_name().c_str());
  } else {
    fk::algorithms::klog("CLOCK MANAGER", "Attempted to set a null clock.");
  }
}

DateTime ClockManager::datetime() {
  return m_clock ? m_clock->datetime() : DateTime{};
}
