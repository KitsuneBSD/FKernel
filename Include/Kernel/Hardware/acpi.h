#pragma once

#include <LibFK/Container/fixed_string.h>
#include <LibFK/Types/types.h>

#include <Kernel/Hardware/ACPI/rsdp.h>
#include <Kernel/Hardware/ACPI/rsdt.h>
#include <Kernel/Hardware/ACPI/xsdt.h>
#include <Kernel/Hardware/ACPI/sdt_header.h>

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

  RSDP *m_rsdp{nullptr};
  RSDT *m_rsdt{nullptr};
  XSDT *m_xsdt{nullptr};

public:
  static ACPIManager &the();

  void initialize();
  void *find_table(const char *signature);
};
