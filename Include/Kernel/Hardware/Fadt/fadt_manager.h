#pragma once

#include <Kernel/Hardware/Fadt/fadt.h>
#include <LibFK/Types/types.h>

class ACPIManager;

class FadtManager {
public:
  static FadtManager &the();

  void initialize(ACPIManager *acpi);
  FADT *get_fadt() const { return m_fadt; }
  uint32_t get_length() const { return m_length; }
  bool has_x_fields() const { return m_has_x_fields; }

  bool get_pm_timer_block(uint32_t &out) const;
  bool get_pm1a_control_block(uint32_t &out) const;
  bool get_x_pm_timer(GenericAddressStructure &out) const;

private:
  FADT *m_fadt{nullptr};
  uint32_t m_length{0};
  bool m_has_x_fields{false};
};
