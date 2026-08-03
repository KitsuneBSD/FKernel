#pragma once

#include <LibFK/Algorithms/Logging/log.h>
#include <LibFK/Text/string.h>
#include <LibFK/Types/types.h>

class PciDevice;

class HardwareInterrupt {
public:
  virtual void initialize() = 0;
  virtual void mask_interrupt(uint8_t interrupt_number) = 0;
  virtual void unmask_interrupt(uint8_t interrupt_number) = 0;
  virtual void send_eoi(uint8_t interrupt_number) = 0;
  virtual fk::text::String get_name() = 0;
  virtual ~HardwareInterrupt() = default;

  virtual fk::core::Result<uint8_t, fk::core::Error> allocate_msi_vector(const PciDevice& device) {
      (void)device; return fk::core::Error::NotImplemented;
  }
  virtual fk::core::Result<uint8_t, fk::core::Error> allocate_msix_vector(const PciDevice& device,
                                                                            uint16_t entry) {
      (void)device; (void)entry; return fk::core::Error::NotImplemented;
  }
  virtual void enable_msi(uint8_t vector) { (void)vector; }
  virtual void disable_msi(uint8_t vector) { (void)vector; }
};

