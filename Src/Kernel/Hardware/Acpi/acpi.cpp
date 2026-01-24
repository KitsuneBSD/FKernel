#include <Kernel/Hardware/Acpi/acpi.h>
#include <Kernel/Hardware/Fadt/fadt.h>
#include <LibC/string.h>
#include <LibFK/Core/Assertions.h>
#include <LibFK/Algorithms/log.h>

ACPIManager &ACPIManager::the() {
  static ACPIManager instance;
  return instance;
}

ACPIManager::ACPIManager() {}
ACPIManager::~ACPIManager() {}

void ACPIManager::initialize() {
  m_rsdp = find_rsdp();
  assert(m_rsdp && "RSDP not found - critical ACPI infrastructure unavailable!");

  if (m_rsdp->revision >= 2 && m_rsdp->xsdt_address) {
    m_xsdt = (XSDT *)((uintptr_t)m_rsdp->xsdt_address);
    assert(validate_checksum(m_xsdt, m_xsdt->header.length) && "XSDT checksum validation failed - corrupted table!");
  }

  if (!m_xsdt) {
    m_rsdt = (RSDT *)((uintptr_t)m_rsdp->rsdt_address);
    assert(validate_checksum(m_rsdt, m_rsdt->header.length) && "RSDT checksum validation failed - corrupted table!");
  }

  // Initialize FADT support via the FadtManager
  extern void initialize_fadt_from_acpi(ACPIManager *);
  initialize_fadt_from_acpi(this);

  initialize_madt();
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


