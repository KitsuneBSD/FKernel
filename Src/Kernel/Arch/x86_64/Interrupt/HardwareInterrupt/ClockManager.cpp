#include <Kernel/Arch/x86_64/Interrupt/HardwareInterrupts/ClockController/cmos.h>
#include <Kernel/Arch/x86_64/Interrupt/HardwareInterrupts/ClockController/rtc.h>
#include <Kernel/Arch/x86_64/Interrupt/HardwareInterrupts/ClockInterrupt.h>
#include <LibFK/Algorithms/log.h>

void ClockManager::initialize() {
  static RTCClock rtc_clock_instance;
  static CMOSClock cmos_clock_instance;
  if (rtc_clock_instance.initialize(1024).is_ok()) {
    set_clock(&rtc_clock_instance);
  } else {
    klog("CLOCK MANAGER", "RTC initialization failed, falling back to CMOS.");
    if (cmos_clock_instance.initialize(0).is_ok()) {
      set_clock(&cmos_clock_instance);
    } else {
      klog("CLOCK MANAGER", "CMOS initialization also failed. No clock set.");
    }
  }
  klog("CLOCK MANAGER", "Clock manager initialized.");
}

void ClockManager::set_clock(Clock *clock) {
  m_clock = clock;
  if (m_clock) {
    klog("CLOCK MANAGER", "Set a new clock called %s",
         m_clock->get_name().c_str());
  } else {
    klog("CLOCK MANAGER", "Attempted to set a null clock.");
  }
}

DateTime ClockManager::datetime() {
  return m_clock ? m_clock->datetime() : DateTime{};
}
