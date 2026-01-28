#pragma once

#include <Kernel/Driver/Storage/Ata/ata_device.h>
#include <Kernel/Driver/Device/driver_manager.h>
#include <Kernel/Hardware/Pci/pci.h>
#include <LibFK/Container/vector.h>
#include <LibFK/Memory/retain_ptr.h>

class ATAController final : public fkernel::Driver {
private:
  fk::containers::Vector<fk::RefPtr<ATADevice>> m_devices;
  ATAController() = default;
  void probe_channel(uint16_t io, uint16_t ctrl, int channel_index);

public:
  static ATAController &the();
  void initialize();

  // Driver interface
  virtual const char* name() const override { return "ATAController"; }
  virtual void probe() override { detect_devices(); }

  void detect_on_pci(const PciDevice &device);
  void detect_legacy();

  void detect_devices();
  const fk::containers::Vector<fk::RefPtr<ATADevice>> &
  devices() const {
    return m_devices;
  }

};
