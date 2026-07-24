#pragma once

#include <Kernel/Driver/Device/CharacterDevice/character_device.h>

namespace fkernel {

// Placeholder for /dev/ptmx existence. Actual open() is handled specially in sys_open.
class PtmxDevice final : public CharacterDevice {
public:
  PtmxDevice() { set_name("ptmx"); }
  virtual ~PtmxDevice() override = default;

  virtual fk::core::Result<size_t, fk::core::Error> read(uint64_t, size_t, uint8_t*) override {
    return fk::core::Error::NotImplemented;
  }
  virtual fk::core::Result<size_t, fk::core::Error> write(uint64_t, size_t, const uint8_t*) override {
    return fk::core::Error::NotImplemented;
  }
  virtual size_t size() const override { return 0; }
};

} // namespace fkernel
