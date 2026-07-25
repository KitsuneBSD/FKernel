#pragma once

#include <Kernel/Arch/x86_64/Interrupt/HardwareInterrupts/hardware_interrupt.h>
#include <LibFK/Types/types.h>

class PciDevice;
class InterruptController;

namespace fkernel {

class HardwareInterruptManager {
  friend class ::InterruptController;

private:
  HardwareInterrupt* m_controller = nullptr;
  bool m_has_memory_manager = false;
  uint32_t m_unmasked_irqs = 0;
  bool m_is_initialized{false};

  HardwareInterruptManager() = default;
  HardwareInterruptManager(const HardwareInterruptManager &) = delete;
  HardwareInterruptManager &operator=(const HardwareInterruptManager &) = delete;

  void select_and_configure_controller();

protected:
  fk::text::String get_name() { return m_controller ? m_controller->get_name() : "None"; }
  HardwareInterrupt* get_m_controller() { return m_controller; }

public:
  static HardwareInterruptManager& the() {
    static HardwareInterruptManager inst;
    return inst;
  }

  bool is_initialized() const { return m_is_initialized; }
  void initialize();
  void mask_interrupt(uint8_t irq);
  void unmask_interrupt(uint8_t irq);
  void send_eoi(uint8_t irq);

  fk::core::Result<uint8_t, fk::core::Error> allocate_msi_vector(const PciDevice& device);
  fk::core::Result<uint8_t, fk::core::Error> allocate_msix_vector(const PciDevice& device,
                                                                    uint16_t entry);
  void enable_msi(uint8_t vector);
  void disable_msi(uint8_t vector);

  void set_controller(HardwareInterrupt* controller);
  void set_memory_manager(bool is_memory_manager);
};

} // namespace fkernel

using fkernel::HardwareInterruptManager;
