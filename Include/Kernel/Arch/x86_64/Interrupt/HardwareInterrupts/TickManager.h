#pragma once

#include <LibFK/Types/types.h>

class TickManager {
private:
  uint64_t m_ticks = 0;
  uint32_t m_frequency = 0;

public:
  TickManager() = default;

  static TickManager &the() {
    static TickManager inst;
    return inst;
  }

  void sleep(uint64_t ms);
  void increment_ticks() { m_ticks++; };
  uint64_t get_ticks() { return m_ticks; }
  void set_frequency(uint32_t frequency) { m_frequency = frequency; }
};
