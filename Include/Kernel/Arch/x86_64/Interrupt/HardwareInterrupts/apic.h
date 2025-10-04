#pragma once

#include <LibC/stdint.h>
#include <LibFK/Algorithms/log.h>

constexpr uint32_t APIC_BASE_MSR = 0x1B;
constexpr uint32_t APIC_ENABLE = 1 << 11;
constexpr uint32_t APIC_BSP = 1 << 8;
constexpr uint32_t APIC_SPURIOUS = 0xF0;
constexpr uint32_t APIC_SVR_ENABLE = 0x100; // Bit 8
constexpr uintptr_t APIC_RANGE_SIZE = 0x1000;

constexpr uint32_t APIC_LVT_TIMER_MODE_PERIODIC = (1 << 17);
constexpr uint32_t APIC_TIMER_DIVISOR = 0x3;

struct local_apic {
  volatile uint32_t id;
  volatile uint32_t version;
  volatile uint32_t tpr;
  volatile uint32_t apr;
  volatile uint32_t ppr;
  volatile uint32_t eoi;
  volatile uint32_t ldr;
  volatile uint32_t dfr;
  volatile uint32_t spurious;
  volatile uint32_t isr[8];
  volatile uint32_t tmr[8];
  volatile uint32_t irr[8];
  volatile uint32_t esr;
  volatile uint32_t icr_low;
  volatile uint32_t icr_high;
  volatile uint32_t lvt_timer;
  volatile uint32_t lvt_thermal;
  volatile uint32_t lvt_perf;
  volatile uint32_t lvt_lint0;
  volatile uint32_t lvt_lint1;
  volatile uint32_t lvt_error;
  volatile uint32_t initial_count;
  volatile uint32_t current_count;
  volatile uint32_t divide_config;
};

class APIC {
private:
  APIC() = default;
  local_apic *lapic = nullptr;
  uint64_t apic_ticks_per_ms = 0;

public:
  static APIC &the() {
    static APIC inst;
    return inst;
  }

  void enable();
  void send_eoi();
  void calibrate_timer();
  void setup_timer(uint64_t ms);
  uint64_t get_ticks_per_ms() const { return apic_ticks_per_ms; }
};
