#pragma once

#include <LibC/stdint.h>
#include <LibFK/log.h>

constexpr uint32_t APIC_BASE_MSR = 0x1B;
constexpr uint32_t APIC_ENABLE = 1 << 11;
constexpr uint32_t APIC_BSP = 1 << 8;
constexpr uint32_t APIC_SPURIOUS = 0xF0;
constexpr uint32_t APIC_SVR_ENABLE = 0x100; // Bit 8

struct local_apic {
  volatile uint32_t id;
  volatile uint32_t version;
  uint32_t reserved0[4];
  volatile uint32_t tpr;
  volatile uint32_t apr;
  volatile uint32_t ppr;
  volatile uint32_t eoi;
};

class APIC {
private:
  APIC() = default;
  local_apic *lapic = nullptr;

public:
  static APIC &the() {
    static APIC inst;
    return inst;
  }

  void enable();
  void send_eoi();
};
