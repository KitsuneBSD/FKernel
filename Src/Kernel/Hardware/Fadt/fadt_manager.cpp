#include <Kernel/Hardware/Fadt/fadt_manager.h>
#include <Kernel/Hardware/Acpi/acpi.h>
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
    return;
  }

  m_length = h->length;
  m_fadt = (FADT *)h;
  size_t base_size = offsetof(FADT, x_firmware_ctrl); // end of 32-bit FADT fields
  m_has_x_fields = m_length >= base_size;
  if (!m_has_x_fields) {
    fk::algorithms::klog("FADT", "FADT found at %p (len=%u) - revision=%u; extended 64-bit fields absent (base_size=%zu, full_size=%zu)\n",
                         m_fadt, m_length, (uint32_t)m_fadt->header.revision, base_size, sizeof(FADT));
  } else {
    fk::algorithms::klog("FADT", "FADT found at %p (len=%u) - revision=%u; full table available (full_size=%zu)\n",
                         m_fadt, m_length, (uint32_t)m_fadt->header.revision, sizeof(FADT));
  }
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

bool FadtManager::get_x_pm_timer(GenericAddressStructure &out) const {
  if (!m_fadt) return false;
  size_t off = offsetof(FADT, x_pm_timer_block);
  if (m_length >= off + sizeof(out)) {
    out = m_fadt->x_pm_timer_block;
    return true;
  }
  return false;
}
