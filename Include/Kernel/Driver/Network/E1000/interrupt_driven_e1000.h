#pragma once
#include <Kernel/Driver/Network/E1000/e1000.h>
#include <Kernel/Hardware/Buses/Pci/pci_device.h>

namespace fkernel {

class InterruptDrivenE1000 : public E1000Controller {
public:
  static fk::RefPtr<InterruptDrivenE1000> create(const PciDevice& device);

  explicit InterruptDrivenE1000(const PciDevice& device)
    : E1000Controller(device), m_interrupt_line(0), m_rx_pending(false) {}

  virtual ~InterruptDrivenE1000() override = default;
  virtual const char* name() const override { return "InterruptDrivenE1000"; }

  void handle_interrupt();
  uint32_t interrupt_line() const { return m_interrupt_line; }

  virtual fk::core::Result<size_t, fk::core::Error>
  receive_packet(uint8_t* buffer, size_t max_size) override;

private:
  static constexpr uint16_t REG_ICR   = 0x00C0;
  static constexpr uint32_t ICR_RXT0  = (1u << 7);  // Receive Timer Interrupt

  uint32_t m_interrupt_line;
  volatile bool m_rx_pending;
};

} // namespace fkernel
