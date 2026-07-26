#include <Kernel/Arch/x86_64/Interrupt/HardwareInterrupts/InterruptController/apic.h>
#include <Kernel/Arch/x86_64/Interrupt/HardwareInterrupts/InterruptController/apic_common.h>
#include <Kernel/Arch/x86_64/Interrupt/HardwareInterrupts/InterruptController/ioapic.h>
#include <Kernel/Arch/x86_64/Interrupt/HardwareInterrupts/InterruptController/msi_helpers.h>
#include <Kernel/Arch/x86_64/Interrupt/HardwareInterrupts/InterruptController/x2apic.h>
#include <Kernel/Hardware/Acpi/acpi.h>
#include <Kernel/Hardware/Cpu/cpu.h>
#include <Kernel/Hardware/Pci/pci_device.h>
#include <Kernel/Memory/memory_manager.h>
#include <LibFK/Algorithms/log.h>

uint32_t IoApicController::read(uint32_t reg) const {
  *reinterpret_cast<volatile uint32_t *>(base) = reg;
  return *reinterpret_cast<volatile uint32_t *>(base + 0x10);
}

void IoApicController::write(uint32_t reg, uint32_t value) const {
  *reinterpret_cast<volatile uint32_t *>(base) = reg;
  *reinterpret_cast<volatile uint32_t *>(base + 0x10) = value;
}

static uint32_t iso_flags_to_ioapic(uint32_t iso_flags) {
  uint32_t flags = 0;
  if ((iso_flags & 0x3) == 3)
    flags |= 1ULL << 13;
  if (((iso_flags >> 2) & 0x3) == 3)
    flags |= 1ULL << 15;
  return flags;
}

const IoApicController *IOAPIC::find_controller_for_gsi(uint32_t gsi) const {
  for (uint32_t i = 0; i < m_controller_count; ++i) {
    uint32_t gsi_end = m_controllers[i].gsi_base + m_controllers[i].max_entries;
    if (gsi >= m_controllers[i].gsi_base && gsi < gsi_end)
      return &m_controllers[i];
  }
  if (m_controller_count > 0)
    return &m_controllers[0];
  return nullptr;
}

void IOAPIC::write_redir_entry(const IoApicController &ctrl, uint32_t gsi, uint64_t entry) {
  uint32_t local_index = gsi - ctrl.gsi_base;
  uint32_t reg_low = IOAPIC_REG_TABLE_BASE + local_index * 2;
  uint32_t reg_high = reg_low + 1;
  ctrl.write(reg_low, static_cast<uint32_t>(entry));
  ctrl.write(reg_high, static_cast<uint32_t>(entry >> 32));
}

void IOAPIC::init_controller(IoApicController &ctrl, uintptr_t address, uint32_t gsi_base, uint8_t lapic_id) {
  ctrl.base = address;
  ctrl.gsi_base = gsi_base;

  MemoryManager::the().map_page(address, address,
                                PageFlags::Present | PageFlags::Writable |
                                    PageFlags::WriteThrough);

  uint32_t ver = ctrl.read(IOAPIC_REG_VER);
  ctrl.max_entries = ((ver >> 16) & 0xFF) + 1;

  fk::algorithms::klog("IOAPIC", "Initializing IOAPIC at %p GSI=%u ver=%u entries=%u dest=%u",
                       address, gsi_base, ver & 0xFF, ctrl.max_entries, lapic_id);

  for (uint32_t i = 0; i < ctrl.max_entries; ++i) {
    uint32_t gsi = gsi_base + i;
    uint8_t vector = 32 + gsi;
    uint64_t entry = (static_cast<uint64_t>(lapic_id) << 56) | vector | IOAPIC_REDIR_MASKED;
    write_redir_entry(ctrl, gsi, entry);
  }
}

void IOAPIC::apply_iso_overrides() {
  uint32_t iso_count = ACPIManager::the().iso_count();
  for (uint32_t i = 0; i < iso_count; ++i) {
    auto *iso = ACPIManager::the().iso(i);
    if (!iso) continue;

    auto *ctrl = find_controller_for_gsi(iso->gsi);
    if (!ctrl) {
      fk::algorithms::kwarn("IOAPIC", "ISO IRQ%u->GSI%u: no IOAPIC covers this GSI",
                            iso->irq_source, iso->gsi);
      continue;
    }

    uint8_t vector = 32 + iso->irq_source;
    uint32_t ioapic_flags = iso_flags_to_ioapic(iso->flags) | IOAPIC_REDIR_MASKED;

    fk::algorithms::klog("IOAPIC", "ISO: IRQ%u -> GSI%u vector=%u flags=0x%x (iso=0x%x)",
                         iso->irq_source, iso->gsi, vector, ioapic_flags, iso->flags);

    m_irq_to_gsi[iso->irq_source] = static_cast<uint8_t>(iso->gsi);
    m_irq_overridden[iso->irq_source] = true;

    uint64_t entry = (static_cast<uint64_t>(CPU::the().lapic_id()) << 56) | vector | static_cast<uint64_t>(ioapic_flags);
    write_redir_entry(*ctrl, iso->gsi, entry);
  }
}

void IOAPIC::initialize() {
  for (uint32_t i = 0; i < MAX_IRQ; ++i)
    m_irq_to_gsi[i] = static_cast<uint8_t>(i);

  uint8_t bsp_lapic_id = CPU::the().lapic_id();

  if (ACPIManager::the().ioapic_count() > 0) {
    for (uint32_t i = 0; i < ACPIManager::the().ioapic_count() && m_controller_count < MAX_IOAPIC_CONTROLLERS; ++i) {
      auto *info = ACPIManager::the().ioapic_info(i);
      if (!info || info->address == 0) continue;
      init_controller(m_controllers[m_controller_count], info->address, info->gsi_base, bsp_lapic_id);
      m_controller_count++;
    }
  }

  if (m_controller_count == 0) {
    uintptr_t acpi_addr = ACPIManager::the().ioapic_address();
    uintptr_t address = (acpi_addr != 0) ? acpi_addr : IOAPIC_ADDRESS;
    fk::algorithms::klog("IOAPIC", "Falling back to single IOAPIC at %p (%s)",
                         address, (acpi_addr != 0) ? "MADT" : "hardcoded");
    init_controller(m_controllers[0], address, 0, bsp_lapic_id);
    m_controller_count = 1;
  }

  apply_iso_overrides();

  fk::algorithms::klog("IOAPIC", "Initialized %u IOAPIC controller(s)", m_controller_count);
}

void IOAPIC::mask_interrupt(uint8_t irq) {
  uint8_t gsi = m_irq_to_gsi[irq];
  auto *ctrl = find_controller_for_gsi(gsi);
  if (!ctrl) return;

  uint32_t local = gsi - ctrl->gsi_base;
  uint32_t reg_low = IOAPIC_REG_TABLE_BASE + local * 2;
  uint32_t reg_high = reg_low + 1;

  uint64_t entry = ((uint64_t)ctrl->read(reg_high) << 32) | ctrl->read(reg_low);
  entry |= IOAPIC_REDIR_MASKED;

  ctrl->write(reg_low, (uint32_t)entry);
  ctrl->write(reg_high, (uint32_t)(entry >> 32));
}

void IOAPIC::unmask_interrupt(uint8_t irq) {
  uint8_t gsi = m_irq_to_gsi[irq];
  auto *ctrl = find_controller_for_gsi(gsi);
  if (!ctrl) return;

  uint32_t local = gsi - ctrl->gsi_base;
  uint32_t reg_low = IOAPIC_REG_TABLE_BASE + local * 2;
  uint32_t reg_high = reg_low + 1;

  uint64_t entry = ((uint64_t)ctrl->read(reg_high) << 32) | ctrl->read(reg_low);
  entry &= ~IOAPIC_REDIR_MASKED;

  ctrl->write(reg_low, (uint32_t)entry);
  ctrl->write(reg_high, (uint32_t)(entry >> 32));
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
  fk::algorithms::kdebug("IOAPIC", "Remap IRQ %u -> vector %u (lapic %u)", irq, vector, lapic_id);

  uint8_t gsi = m_irq_to_gsi[irq];
  auto *ctrl = find_controller_for_gsi(gsi);
  if (!ctrl) {
    fk::algorithms::kwarn("IOAPIC", "Cannot remap IRQ %u (GSI %u): no controller found", irq, gsi);
    return;
  }

  uint32_t local = gsi - ctrl->gsi_base;
  uint64_t entry = vector | ((uint64_t)lapic_id << 56) | static_cast<uint64_t>(flags);

  uint32_t reg_low = IOAPIC_REG_TABLE_BASE + local * 2;
  uint32_t reg_high = reg_low + 1;

  ctrl->write(reg_low, (uint32_t)entry);
  ctrl->write(reg_high, (uint32_t)(entry >> 32));
}

fk::core::Result<uint8_t, fk::core::Error>
IOAPIC::allocate_msi_vector(const PciDevice& device) {
  return msi::allocate_msi_vector(device);
}

void IOAPIC::enable_msi(uint8_t vector) {
    (void)vector;
}

void IOAPIC::disable_msi(uint8_t vector) {
    (void)vector;
}
