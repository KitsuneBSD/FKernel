#pragma once

#include <LibFK/Container/string.h>
#include <LibFK/Types/types.h>

class HardwareInterrupt {
public:
  virtual void initialize() = 0;
  virtual void mask_interrupt(uint8_t interrupt_number) = 0;
  virtual void unmask_interrupt(uint8_t interrupt_number) = 0;
  virtual void send_eoi(uint8_t interrupt_number) = 0;
  virtual String get_name() = 0;
  virtual ~HardwareInterrupt() = default;
};
