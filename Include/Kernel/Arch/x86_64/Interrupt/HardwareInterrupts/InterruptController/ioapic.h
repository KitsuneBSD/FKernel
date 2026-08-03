#pragma once

#include <Kernel/Arch/x86_64/Interrupt/HardwareInterrupts/InterruptController/ioapic_controller.h>
#include <Kernel/Arch/x86_64/Interrupt/HardwareInterrupts/hardware_interrupt.h>
#include <LibFK/Text/string.h>
#include <LibFK/Types/types.h>

// I/O APIC Registers
constexpr uint32_t IOAPIC_REG_ID = 0x00;
constexpr uint32_t IOAPIC_REG_VER = 0x01;
constexpr uint32_t IOAPIC_REG_ARB = 0x02;
constexpr uint32_t IOAPIC_REG_TABLE_BASE = 0x10;
constexpr uintptr_t IOAPIC_ADDRESS = 0xFEC00000;

static constexpr uint32_t MAX_IOAPIC_CONTROLLERS = 8;

/**
 * @brief I/O APIC controller implementing HardwareInterrupt interface
 */
class IOAPIC : public HardwareInterrupt {
private:
  IoApicController m_controllers[MAX_IOAPIC_CONTROLLERS];
  uint32_t m_controller_count{0};

  fk::text::String m_name = "IOAPIC";

  static constexpr uint64_t IOAPIC_REDIR_MASKED     = 1ULL << 16;
  static constexpr uint64_t IOAPIC_REDIR_POLARITY_LOW = 1ULL << 13;
  static constexpr uint64_t IOAPIC_REDIR_TRIGGER_LEVEL = 1ULL << 15;
  static constexpr uint32_t MAX_IRQ = 256;

  uint8_t m_irq_to_gsi[MAX_IRQ]{};
  bool m_irq_overridden[MAX_IRQ]{};

  void init_controller(IoApicController &ctrl, uintptr_t address, uint32_t gsi_base, uint8_t lapic_id);
  void apply_iso_overrides();
  const IoApicController *find_controller_for_gsi(uint32_t gsi) const;
  void write_redir_entry(const IoApicController &ctrl, uint32_t gsi, uint64_t entry);

public:
  IOAPIC() = default;

  fk::text::String get_name() override { return m_name; }
  void initialize() override;
  void mask_interrupt(uint8_t irq) override;
  void unmask_interrupt(uint8_t irq) override;
  void send_eoi(uint8_t irq) override;

  fk::core::Result<uint8_t, fk::core::Error> allocate_msi_vector(const PciDevice& device) override;
  void enable_msi(uint8_t vector) override;
  void disable_msi(uint8_t vector) override;

  /**
   * @brief Remap IRQ to vector and LAPIC
   */
  void remap_irq(uint8_t irq, uint8_t vector, uint8_t lapic_id, uint32_t flags);
};
