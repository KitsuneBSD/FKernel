#pragma once

#include <LibFK/Text/fixed_string.h>
#include <LibFK/Types/types.h>

#include <Kernel/Hardware/Acpi/rsdp.h>
#include <Kernel/Hardware/Acpi/rsdt.h>
#include <Kernel/Hardware/Acpi/xsdt.h>
#include <Kernel/Hardware/Acpi/sdt_header.h>
#include <Kernel/Hardware/Madt/madt.h>
#include <Kernel/Hardware/Madt/madt_entries.h>

/**
 *  @brief ACPI Manager
 * This class manages the ACPI subsystem, providing methods to initialize
 * the ACPI tables and find specific ACPI tables by their signature.
 * It uses the RSDP to locate the RSDT or XSDT, and then allows
 * querying for specific ACPI tables. 
**/
class ACPIManager {
private:
  ACPIManager();
  ~ACPIManager();

  static bool validate_checksum(const void *table, size_t length);
  static RSDP *find_rsdp();
  void initialize_madt();
  void process_madt_entries();

  RSDP *m_rsdp{nullptr};
  RSDT *m_rsdt{nullptr};
  XSDT *m_xsdt{nullptr};
  Madt *m_madt{nullptr};

public:
  static ACPIManager &the();

  void initialize();
  void *find_table(const char *signature);
  Madt *get_madt() const { return m_madt; }
};
