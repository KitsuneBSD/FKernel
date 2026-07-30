#pragma once

#include <Kernel/Hardware/Fadt/fadt.h>
#include <LibFK/Types/types.h>

namespace fkernel { class ACPIManager; }

namespace fkernel {

class FadtManager {
private:
  FADT *m_fadt{nullptr};
  uint32_t m_length{0};
  bool m_has_x_fields{false};
  bool m_is_initialized{false};

  FadtManager() = default;
  FadtManager(const FadtManager &) = delete;
  FadtManager &operator=(const FadtManager &) = delete;

public:
  static FadtManager &the();

  bool is_initialized() const { return m_is_initialized; }
  void initialize(ACPIManager *acpi);

  FADT *get_fadt() const { return m_fadt; }
  uint32_t get_length() const { return m_length; }
  bool has_x_fields() const { return m_has_x_fields; }

  bool validate_ports() const;

  bool get_pm_timer_block(uint32_t &out) const;
  bool get_pm1a_control_block(uint32_t &out) const;
  bool get_pm1b_control_block(uint32_t &out) const;
  bool get_smi_command_port(uint32_t &out) const;

  bool get_x_pm_timer(GenericAddressStructure &out) const;

  bool get_reset_register(GenericAddressStructure &out) const;
  bool get_reset_value(uint8_t &out) const;
};

} // namespace fkernel
using fkernel::FadtManager;
