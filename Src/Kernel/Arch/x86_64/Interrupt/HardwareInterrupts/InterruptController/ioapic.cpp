#include <Kernel/Arch/x86_64/Interrupt/HardwareInterrupts/InterruptController/apic.h>
#include <Kernel/Arch/x86_64/Interrupt/HardwareInterrupts/InterruptController/ioapic.h>
#include <Kernel/Hardware/Acpi/acpi.h>
#include <Kernel/Arch/x86_64/Interrupt/HardwareInterrupts/InterruptController/x2apic.h>
#include <Kernel/Hardware/Cpu/cpu.h>
#include <Kernel/Hardware/Pci/pci_device.h>
#include <Kernel/Memory/memory_manager.h>
#include <LibFK/Algorithms/log.h>

// I/O APIC Redirection Table Entry bits
constexpr uint64_t IOAPIC_REDIR_MASKED = 1ULL << 16;

uint32_t IOAPIC::read(uint32_t reg) {
  *reinterpret_cast<volatile uint32_t *>(ioapic_base) = reg;
  return *reinterpret_cast<volatile uint32_t *>(ioapic_base + 0x10);
}

void IOAPIC::write(uint32_t reg, uint32_t value) {
  *reinterpret_cast<volatile uint32_t *>(ioapic_base) = reg;
  *reinterpret_cast<volatile uint32_t *>(ioapic_base + 0x10) = value;
}

void IOAPIC::initialize() {
  uintptr_t acpi_addr = ACPIManager::the().ioapic_address();
  ioapic_base = (acpi_addr != 0) ? acpi_addr : IOAPIC_ADDRESS;
  fk::algorithms::klog("IOAPIC", "Initializing IOAPIC at %p (%s)",
                       ioapic_base, (acpi_addr != 0) ? "MADT" : "hardcoded");

  // Map I/O APIC registers
  MemoryManager::the().map_page(ioapic_base, ioapic_base,
                                PageFlags::Present | PageFlags::Writable |
                                    PageFlags::WriteThrough);
  uint32_t ver = read(IOAPIC_REG_VER);
  uint32_t max_entries = ((ver >> 16) & 0xFF) + 1;

  fk::algorithms::klog("IOAPIC", "Version %u, %u redirection entries",
                       ver & 0xFF, max_entries);

  // Initialize all entries with default mapping (Vector = 32 + IRQ, Dest=BSP) but MASKED
  for (uint32_t i = 0; i < max_entries; ++i) {
    // Vector starts at 32 (0x20) for IRQ0
    uint8_t vector = 32 + i;
    // Default: Edge Triggered (0), Active High (0), Physical Dest (0), Fixed Delivery (0)
    // We only set the Masked bit initially.
    remap_irq(i, vector, 0, IOAPIC_REDIR_MASKED);
  }
}

void IOAPIC::mask_interrupt(uint8_t irq) {
  uint32_t reg_low = IOAPIC_REG_TABLE_BASE + irq * 2;
  uint32_t reg_high = reg_low + 1;

  uint64_t entry = ((uint64_t)read(reg_high) << 32) | read(reg_low);
  entry |= IOAPIC_REDIR_MASKED;

  write(reg_low, (uint32_t)entry);
  write(reg_high, (uint32_t)(entry >> 32));
}

void IOAPIC::unmask_interrupt(uint8_t irq) {
  uint32_t reg_low = IOAPIC_REG_TABLE_BASE + irq * 2;
  uint32_t reg_high = reg_low + 1;

  uint64_t entry = ((uint64_t)read(reg_high) << 32) | read(reg_low);
  entry &= ~IOAPIC_REDIR_MASKED;

  write(reg_low, (uint32_t)entry);
  write(reg_high, (uint32_t)(entry >> 32));
}

void IOAPIC::send_eoi([[maybe_unused]] uint8_t irq) {
  if (CPU::the().has_x2apic()) {
    X2APIC::the().send_eoi(irq);
  } else {
    APIC::the().send_eoi(irq);
  }
}

void IOAPIC::remap_irq(uint8_t irq, uint8_t vector, uint8_t lapic_id,
                       uint32_t flags) {
  uint64_t entry = vector | ((uint64_t)lapic_id << 56) | flags;

  uint32_t reg_low = IOAPIC_REG_TABLE_BASE + irq * 2;
  uint32_t reg_high = reg_low + 1;

  write(reg_low, (uint32_t)entry);
  write(reg_high, (uint32_t)(entry >> 32));
}

extern uint8_t g_next_msi_vector;

fk::core::Result<uint8_t, fk::core::Error> IOAPIC::allocate_msi_vector(const PciDevice& device) {
    uint8_t msi_ptr = device.find_capability(0x05); // MSI Capability ID
    if (msi_ptr == 0) {
        fk::algorithms::kwarn("MSI", "Device %02x:%02x.%d does not support MSI", 
                             device.address().bus(), device.address().device(), 
                             device.address().function());
        return fk::core::Error::NotImplemented;
    }

    uint8_t vector = g_next_msi_vector++;
    if (vector >= 0xF0) return fk::core::Error::OutOfMemory;

    uint32_t msi_addr = static_cast<uint32_t>(APIC::the().msi_address_base());
    if (msi_addr == 0)
        msi_addr = 0xFEE00000;
    device.write_config_dword(msi_ptr + 4, msi_addr);

    uint16_t msg_ctrl = device.read_config_word(msi_ptr + 2);
    if (msg_ctrl & (1 << 7)) { // 64-bit support
        device.write_config_dword(msi_ptr + 8, 0);
        device.write_config_word(msi_ptr + 12, vector);
    } else {
        device.write_config_word(msi_ptr + 8, vector);
    }

    msg_ctrl |= 0x01;
    device.write_config_word(msi_ptr + 2, msg_ctrl);

    fk::algorithms::klog("MSI", "Enabled MSI (Vector 0x%x) for device %02x:%02x.%d", 
                         vector, device.address().bus(), device.address().device(), 
                         device.address().function());

    return vector;
}

void IOAPIC::enable_msi(uint8_t vector) {
    (void)vector;
    // MSI is enabled on the device side during allocation.
    // In a multi-CPU system, we might need to update destination LAPIC.
}

void IOAPIC::disable_msi(uint8_t vector) {
    (void)vector;
    // TODO: Track which device owns this vector to disable it
}
