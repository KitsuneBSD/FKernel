#include <Kernel/Arch/x86_64/Interrupt/HardwareInterrupts/ClockController/rtc.h>
#include <Kernel/Arch/x86_64/Interrupt/HardwareInterrupts/ClockInterrupt.h>
#include <LibFK/Algorithms/log.h>

void ClockManager::initialize() {
  static RTCClock rtc_clock_instance;
  set_clock(&rtc_clock_instance);
  klog("CLOCK MANAGER", "Clock initialized");
}

void ClockManager::set_clock(Clock *clock) {
  m_clock = clock;
  if (m_clock) {
    m_clock->initialize(1024);
    klog("CLOCK MANAGER", "Set a new clock called %s",
         m_clock->get_name().c_str());
  } else {
    klog("CLOCK MANAGER", "Attempted to set a null clock.");
  }
}

DateTime ClockManager::datetime() {
  return m_clock ? m_clock->datetime() : DateTime{};
}
