#pragma once

#include <LibFK/Types/types.h>

struct [[gnu::packed]] RSDP {
  char signature[8];
  uint8_t checksum;
  char oem_id[6];
  uint8_t revision;
  uint32_t rsdt_address;
  // ACPI 2.0 fields
  uint32_t length;
  uint64_t xsdt_address;
  uint8_t extended_checksum;
  uint8_t reserved[3];
};

struct [[gnu::packed]] SDTHeader {
  char signature[4];
  uint32_t length;
  uint8_t revision;
  uint8_t checksum;
  char oem_id[6];
  char oem_table_id[8];
  uint32_t oem_revision;
  uint32_t creator_id;
  uint32_t creator_revision;
};

struct [[gnu::packed]] RSDT {
  SDTHeader header;
  uint32_t sdt_pointers[];
};

struct [[gnu::packed]] XSDT {
  SDTHeader header;
  uint64_t sdt_pointers[];
};

class ACPIManager {
public:
  static ACPIManager &the();

  void initialize();
  void *find_table(const char *signature);

private:
  ACPIManager();
  ~ACPIManager();

  static bool validate_checksum(const void *table, size_t length);
  static RSDP *find_rsdp();

  RSDP *m_rsdp{nullptr};
  RSDT *m_rsdt{nullptr};
  XSDT *m_xsdt{nullptr};
};
