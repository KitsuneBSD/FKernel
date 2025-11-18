#include <Kernel/Arch/x86_64/Interrupt/HardwareInterrupts/ClockController/rtc.h>
#include <Kernel/Arch/x86_64/Interrupt/HardwareInterrupts/ClockInterrupt.h>
#include <LibFK/Utilities/converter.h>

fk::text::String DateTime::to_string() const {
  char buffer[32];
  snprintf(buffer, sizeof(buffer), "%02u/%02u/%04u %02u:%02u:%02u", day, month,
           year, hour, minute, second);
  return fk::text::String(buffer);
}

void DateTime::print() {
  fk::algorithms::klog("CLOCK MANAGER", "%02u/%02u/%04u %02u:%02u:%02u", day,
                       month, year, hour, minute, second);
}
