#include <LibFK/Algorithms/Logging/log.h>

#include <Kernel/Clock/ClockController/cmos.h>
#include <Kernel/Clock/ClockController/rtc.h>
#include <Kernel/Clock/clock_interrupt.h>

void ClockManager::initialize() {
  select_and_configure_clock();
  m_is_initialized = true;
  fk::algorithms::klog("CLOCK_MANAGER", "Clock manager initialized.");
}

static constexpr uint32_t RTC_DEFAULT_FREQUENCY = 1024;

void ClockManager::select_and_configure_clock() {
  static RTCClock rtc_clock_instance;
  static CMOSClock cmos_clock_instance;

  Clock *new_clock = nullptr;
  fk::text::String clock_name = "None";

  if (rtc_clock_instance.initialize(RTC_DEFAULT_FREQUENCY).is_ok()) {
    new_clock = &rtc_clock_instance;
    clock_name = "RTC";
  } else {
    fk::algorithms::klog("CLOCK_MANAGER",
                         "RTC initialization failed, falling back to CMOS.");
    if (cmos_clock_instance.initialize(0).is_ok()) {
      new_clock = &cmos_clock_instance;
      clock_name = "CMOS";
    } else {
      fk::algorithms::klog("CLOCK_MANAGER",
                           "CMOS initialization also failed. No clock set.");
    }
  }

  if (new_clock != m_clock) {
    set_clock(new_clock);
    fk::algorithms::klog("CLOCK_MANAGER", "Clock controller set to: %s",
                         clock_name.c_str());
  } else {
    fk::algorithms::kdebug("CLOCK_MANAGER", "Clock controller remains: %s",
                           clock_name.c_str());
  }
}

void ClockManager::set_clock(Clock *clock) {
  m_clock = clock;
  if (m_clock) {
    fk::algorithms::klog("CLOCK_MANAGER", "Set a new clock called %s",
                         m_clock->get_name().c_str());
  } else {
    fk::algorithms::klog("CLOCK_MANAGER", "Attempted to set a null clock.");
  }
}

DateTime ClockManager::datetime() {
  return m_clock ? m_clock->datetime() : DateTime{};
}
