#include <Kernel/Arch/x86_64/Interrupt/HardwareInterrupts/InterruptController/msi_helpers.h>
#include <Kernel/Arch/x86_64/Interrupt/HardwareInterrupts/InterruptController/apic_common.h>
#include <Kernel/Hardware/Cpu/cpu.h>
#include <Kernel/Hardware/Pci/pci_device.h>
#include <LibFK/Algorithms/log.h>

static uint32_t g_next_msi_vector = 0x40;

uint32_t msi::lapic_phys_address() {
  return static_cast<uint32_t>(CPU::the().read_msr(APIC_BASE_MSR) & 0xFFFFF000ULL);
}

fk::core::Result<uint8_t, fk::core::Error>
msi::allocate_msi_vector(const PciDevice& device) {
  return msi::write_msi_config(device, msi::lapic_phys_address());
}

fk::core::Result<uint8_t, fk::core::Error> msi::allocate_vector() {
  uint8_t vector = static_cast<uint8_t>(__sync_fetch_and_add(&g_next_msi_vector, 1u));
  if (vector >= 0xF0)
    return fk::core::Error::OutOfMemory;
  return vector;
}

fk::core::Result<uint8_t, fk::core::Error>
msi::write_msi_config(const PciDevice& device, uint32_t msi_addr) {
  uint8_t msi_ptr = device.find_capability(0x05);
  if (msi_ptr == 0) {
    fk::algorithms::kwarn("MSI", "Device %02x:%02x.%d does not support MSI",
                          device.address().bus(), device.address().device(),
                          device.address().function());
    return fk::core::Error::NotImplemented;
  }

  auto vector_result = msi::allocate_vector();
  if (!vector_result.is_ok())
    return vector_result.error();
  uint8_t vector = vector_result.value();

  device.write_config_dword(msi_ptr + 4, msi_addr);

  uint16_t msg_ctrl = device.read_config_word(msi_ptr + 2);
  if (msg_ctrl & (1 << 7)) {
    device.write_config_dword(msi_ptr + 8, 0);
    device.write_config_word(msi_ptr + 12, vector);
  } else {
    device.write_config_word(msi_ptr + 8, vector);
  }

  msg_ctrl |= 0x01;
  device.write_config_word(msi_ptr + 2, msg_ctrl);

  fk::algorithms::klog("MSI", "Enabled MSI (Vector 0x%x) at 0x%x for device %02x:%02x.%d",
                       vector, msi_addr,
                       device.address().bus(), device.address().device(),
                       device.address().function());

  return vector;
}
