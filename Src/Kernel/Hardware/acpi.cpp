#include <Kernel/Hardware/acpi.h>
#include <Kernel/MemoryManager/VirtualMemoryManager.h>
#include <LibC/string.h>
#include <LibFK/Algorithms/log.h>

ACPIManager &ACPIManager::the() {
  static ACPIManager instance;
  return instance;
}

ACPIManager::ACPIManager() {}
ACPIManager::~ACPIManager() {}

void ACPIManager::initialize() {
  m_rsdp = find_rsdp();
  if (!m_rsdp) {
    kwarn("ACPI", "RSDP not found!");
    return;
  }

  kdebug("ACPI", "RSDP found at %p", m_rsdp);

  if (m_rsdp->revision >= 2 && m_rsdp->xsdt_address) {
    m_xsdt = (XSDT *)((uintptr_t)m_rsdp->xsdt_address);
    if (!validate_checksum(m_xsdt, m_xsdt->header.length)) {
      kwarn("ACPI", "XSDT checksum failed!");
      m_xsdt = nullptr;
    } else {
      klog("ACPI", "XSDT found at %p", m_xsdt);
    }
  }

  if (!m_xsdt) {
    m_rsdt = (RSDT *)((uintptr_t)m_rsdp->rsdt_address);
    if (!validate_checksum(m_rsdt, m_rsdt->header.length)) {
      kwarn("ACPI", "RSDT checksum failed!");
      m_rsdt = nullptr;
    } else {
      klog("ACPI", "RSDT found at %p", m_rsdt);
    }
  }
}

void *ACPIManager::find_table(const char *signature) {
  if (m_xsdt) {
    int entries = (m_xsdt->header.length - sizeof(SDTHeader)) / 8;
    for (int i = 0; i < entries; i++) {
      SDTHeader *h = (SDTHeader *)((uintptr_t)m_xsdt->sdt_pointers[i]);
      if (strncmp(h->signature, signature, 4) == 0) {
        if (validate_checksum(h, h->length)) {
          return h;
        }
      }
    }
  } else if (m_rsdt) {
    int entries = (m_rsdt->header.length - sizeof(SDTHeader)) / 4;
    for (int i = 0; i < entries; i++) {
      SDTHeader *h = (SDTHeader *)((uintptr_t)m_rsdt->sdt_pointers[i]);
      if (strncmp(h->signature, signature, 4) == 0) {
        if (validate_checksum(h, h->length)) {
          return h;
        }
      }
    }
  }
  return nullptr;
}

bool ACPIManager::validate_checksum(const void *table, size_t length) {
  uint8_t sum = 0;
  const uint8_t *data = (const uint8_t *)table;
  for (size_t i = 0; i < length; i++) {
    sum += data[i];
  }
  return sum == 0;
}

RSDP *ACPIManager::find_rsdp() {
  // Search in main BIOS area (0xE0000 to 0xFFFFF)
  for (char *p = (char *)0xE0000; p < (char *)0xFFFFF; p += 16) {
    if (strncmp(p, "RSD PTR ", 8) == 0) {
      RSDP *rsdp = (RSDP *)p;
      if (validate_checksum(rsdp, 20)) {
        if (rsdp->revision >= 2) {
          if (validate_checksum(rsdp, rsdp->length)) {
            return rsdp;
          }
        } else {
          return rsdp;
        }
      }
    }
  }
  return nullptr;
}
