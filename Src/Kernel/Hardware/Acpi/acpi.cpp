#include <Kernel/Hardware/Acpi/acpi.h>
#include <Kernel/Hardware/Fadt/fadt.h>
#include <LibFK/Utilities/memory.h>
#include <LibFK/Algorithms/byte_checksum.h>
#include <LibFK/Algorithms/log.h>
#include <LibFK/Core/assertions.h>

extern void initialize_fadt_from_acpi(fkernel::ACPIManager *);

ACPIManager &ACPIManager::the() {
  static ACPIManager instance;
  return instance;
}

ACPIManager::ACPIManager() {}
ACPIManager::~ACPIManager() {}

void ACPIManager::initialize() {
  m_rsdp = find_rsdp();
  assert(m_rsdp && "RSDP not found - critical ACPI infrastructure unavailable!");

  fk::algorithms::klog("ACPI", "RSDP found at %p (Revision: %u)", m_rsdp, (uint32_t)m_rsdp->revision);

  if (m_rsdp->revision >= 2 && m_rsdp->xsdt_address) {
    m_xsdt = (XSDT *)((uintptr_t)m_rsdp->xsdt_address);
    if (validate_checksum(m_xsdt, m_xsdt->header.length)) {
      fk::algorithms::klog("ACPI", "Using XSDT at %p (Length: %u)", m_xsdt, m_xsdt->header.length);
    } else {
      fk::algorithms::klog("ACPI", "XSDT checksum invalid, falling back to RSDT if possible");
      m_xsdt = nullptr;
    }
  }

  if (!m_xsdt) {
    m_rsdt = (RSDT *)((uintptr_t)m_rsdp->rsdt_address);
    assert(validate_checksum(m_rsdt, m_rsdt->header.length) && "RSDT checksum validation failed - corrupted table!");
    fk::algorithms::klog("ACPI", "Using RSDT at %p (Length: %u)", m_rsdt, m_rsdt->header.length);
  }

  // Initialize FADT support via the FadtManager
  initialize_fadt_from_acpi(this);

  initialize_madt();
  m_is_initialized = true;
}


void *ACPIManager::find_table(const char *signature) {
  if (m_xsdt) {
    int entries = (m_xsdt->header.length - sizeof(SDTHeader)) / 8;
    for (int i = 0; i < entries; i++) {
      SDTHeader *h = (SDTHeader *)((uintptr_t)m_xsdt->sdt_pointers[i]);
      if (fk::memory::compare_n(h->signature, signature, 4) == 0) {
        if (validate_checksum(h, h->length)) {
          return h;
        }
      }
    }
  } else if (m_rsdt) {
    int entries = (m_rsdt->header.length - sizeof(SDTHeader)) / 4;
    for (int i = 0; i < entries; i++) {
      SDTHeader *h = (SDTHeader *)((uintptr_t)m_rsdt->sdt_pointers[i]);
      if (fk::memory::compare_n(h->signature, signature, 4) == 0) {
        if (validate_checksum(h, h->length)) {
          return h;
        }
      }
    }
  }
  return nullptr;
}

bool ACPIManager::validate_checksum(const void *table, size_t length) {
  return fk::algorithms::byte_checksum_valid(table, length);
}

RSDP *ACPIManager::find_rsdp() {
  for (char *p = (char *)0xE0000; p < (char *)0xFFFFF; p += 16) {
    if (fk::memory::compare_n(p, "RSD PTR ", 8) == 0) {
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


