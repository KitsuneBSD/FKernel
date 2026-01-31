#include <Kernel/Hardware/Fadt/fadt_manager.h>
#include <Kernel/Hardware/Acpi/acpi.h>
#include <Kernel/Arch/x86_64/io.h>
#include <LibFK/Algorithms/log.h>

FadtManager &FadtManager::the() {
  static FadtManager instance;
  return instance;
}

// Helper function to allow acpi.cpp to call without including the manager header there
void initialize_fadt_from_acpi(ACPIManager *acpi) {
  FadtManager::the().initialize(acpi);
}

void FadtManager::initialize(ACPIManager *acpi) {
  if (!acpi) {
    m_fadt = nullptr;
    return;
  }

  SDTHeader *h = (SDTHeader *)acpi->find_table("FACP");
  if (!h) {
    m_fadt = nullptr;
    m_length = 0;
    fk::algorithms::klog("FADT", "FADT (FACP) table not found!");
    return;
  }

  m_length = h->length;
  m_fadt = (FADT *)h;
  size_t base_size = offsetof(FADT, x_firmware_ctrl); // end of 32-bit FADT fields
  m_has_x_fields = m_length >= base_size;

  fk::algorithms::klog("FADT", "FADT found at %p (len=%u), Revision: %u", 
                       m_fadt, m_length, (uint32_t)m_fadt->header.revision);

  uint32_t port;
  if (get_smi_command_port(port) && port != 0)
    fk::algorithms::klog("FADT", "  SMI Command Port: 0x%x", port);
  if (get_pm_timer_block(port) && port != 0)
    fk::algorithms::klog("FADT", "  PM Timer Block: 0x%x", port);
  if (get_pm1a_control_block(port) && port != 0)
    fk::algorithms::klog("FADT", "  PM1a Control Block: 0x%x", port);
  if (get_pm1b_control_block(port) && port != 0)
    fk::algorithms::klog("FADT", "  PM1b Control Block: 0x%x", port);

  if (!m_has_x_fields) {
    fk::algorithms::klog("FADT", "  Extended 64-bit fields are absent");
  } else {
    fk::algorithms::klog("FADT", "  Extended 64-bit fields are present");
  }

  validate_ports();
}

bool FadtManager::validate_ports() const {
  if (!m_fadt) return false;

  uint32_t timer_port;
  if (get_pm_timer_block(timer_port) && timer_port != 0) {
    uint32_t val1 = inl(timer_port);
    // Wait a bit and read again to see if it changes
    for (int i = 0; i < 10000; i++) {
        asm volatile("" ::: "memory");
    }
    uint32_t val2 = inl(timer_port);
    
    if (val1 == val2) {
      fk::algorithms::klog("FADT", "  [TEST] PM Timer at port 0x%x seems STATIC (val=0x%x)", timer_port, val1);
    } else {
      fk::algorithms::klog("FADT", "  [TEST] PM Timer at port 0x%x is ACTIVE (0x%x -> 0x%x)", timer_port, val1, val2);
    }
  }

  uint32_t smi_port;
  if (get_smi_command_port(smi_port) && smi_port != 0) {
    // We don't want to actually trigger SMI here, just check if we can read it (if it was a read/write port)
    // Most SMI ports are write-only for commands, but let's just log its presence.
    fk::algorithms::klog("FADT", "  [TEST] SMI Command Port 0x%x is present", smi_port);
  }

  return true;
}

bool FadtManager::get_pm_timer_block(uint32_t &out) const {
  if (!m_fadt) return false;
  size_t off = offsetof(FADT, pm_timer_block);
  if (m_length >= off + sizeof(m_fadt->pm_timer_block)) {
    out = m_fadt->pm_timer_block;
    return true;
  }
  return false;
}

bool FadtManager::get_pm1a_control_block(uint32_t &out) const {
  if (!m_fadt) return false;
  size_t off = offsetof(FADT, pm1a_control_block);
  if (m_length >= off + sizeof(m_fadt->pm1a_control_block)) {
    out = m_fadt->pm1a_control_block;
    return true;
  }
  return false;
}

bool FadtManager::get_pm1b_control_block(uint32_t &out) const {
  if (!m_fadt) return false;
  size_t off = offsetof(FADT, pm1b_control_block);
  if (m_length >= off + sizeof(m_fadt->pm1b_control_block)) {
    out = m_fadt->pm1b_control_block;
    return true;
  }
  return false;
}

bool FadtManager::get_smi_command_port(uint32_t &out) const {
  if (!m_fadt) return false;
  size_t off = offsetof(FADT, smi_command_port);
  if (m_length >= off + sizeof(m_fadt->smi_command_port)) {
    out = m_fadt->smi_command_port;
    return true;
  }
  return false;
}

bool FadtManager::get_x_pm_timer(GenericAddressStructure &out) const {
  if (!m_fadt) return false;
  size_t off = offsetof(FADT, x_pm_timer_block);
  if (m_length >= off + sizeof(out)) {
    out = m_fadt->x_pm_timer_block;
    return true;
  }
  return false;
}
