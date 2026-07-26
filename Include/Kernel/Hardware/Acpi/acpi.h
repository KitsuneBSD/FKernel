#pragma once

#include <LibFK/Text/fixed_string.h>
#include <LibFK/Types/types.h>

#include <Kernel/Hardware/Acpi/rsdp.h>
#include <Kernel/Hardware/Acpi/rsdt.h>
#include <Kernel/Hardware/Acpi/xsdt.h>
#include <Kernel/Hardware/Acpi/sdt_header.h>
#include <Kernel/Hardware/Madt/madt.h>
#include <Kernel/Hardware/Madt/madt_entries.h>
#include <Kernel/Hardware/Madt/madt_interrupt_source_override.h>
#include <Kernel/Hardware/Madt/madt_ioapic.h>

/**
 *  @brief ACPI Manager
 * This class manages the ACPI subsystem, providing methods to initialize
 * the ACPI tables and find specific ACPI tables by their signature.
 * It uses the RSDP to locate the RSDT or XSDT, and then allows
 * querying for specific ACPI tables. 
**/
namespace fkernel {

struct IoApicInfo {
  uint8_t id;
  uint32_t address;
  uint32_t gsi_base;
};

class ACPIManager {
  bool m_is_initialized{false};

private:
  ACPIManager();
  ~ACPIManager();
  ACPIManager(const ACPIManager &) = delete;
  ACPIManager &operator=(const ACPIManager &) = delete;

  static bool validate_checksum(const void *table, size_t length);
  static RSDP *find_rsdp();
  void initialize_madt();
  void process_madt_entries();

  RSDP *m_rsdp{nullptr};
  RSDT *m_rsdt{nullptr};
  XSDT *m_xsdt{nullptr};
  Madt *m_madt{nullptr};
  uintptr_t m_ioapic_address{0xFEC00000};

  uint32_t m_cpu_count{0};
  uint8_t m_cpu_apic_ids[32]{};
  static constexpr uint32_t MAX_CPU_APIC_IDS = 32;

  MADT_InterruptSourceOverride m_isos[16]{};
  uint32_t m_iso_count{0};
  static constexpr uint32_t MAX_ISOS = 16;

  IoApicInfo m_ioapics[8]{};
  uint32_t m_ioapic_count{0};
  static constexpr uint32_t MAX_IOAPICS = 8;

public:
  static ACPIManager &the();

  bool is_initialized() const { return m_is_initialized; }
  void initialize(RSDP *external_rsdp = nullptr);
  void *find_table(const char *signature);
  Madt *get_madt() const { return m_madt; }
  uintptr_t ioapic_address() const { return m_ioapic_address; }
  void set_ioapic_address(uintptr_t addr) { m_ioapic_address = addr; }

  uint32_t cpu_count() const { return m_cpu_count; }
  uint8_t cpu_apic_id(uint32_t index) const {
    return (index < m_cpu_count) ? m_cpu_apic_ids[index] : 0;
  }

  uint32_t iso_count() const { return m_iso_count; }
  const MADT_InterruptSourceOverride *iso(uint32_t index) const {
    return (index < m_iso_count) ? &m_isos[index] : nullptr;
  }

  uint32_t ioapic_count() const { return m_ioapic_count; }
  const IoApicInfo *ioapic_info(uint32_t index) const {
    return (index < m_ioapic_count) ? &m_ioapics[index] : nullptr;
  }
};

} // namespace fkernel

using fkernel::ACPIManager;
