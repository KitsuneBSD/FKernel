#pragma once

#include <LibFK/Types/types.h>

namespace fkernel {

class TickManager {
private:
  uint64_t m_ticks = 0;
  uint32_t m_frequency = 0;
  bool m_is_initialized{false};

  TickManager() = default;
  TickManager(const TickManager &) = delete;
  TickManager &operator=(const TickManager &) = delete;

public:
  static TickManager &the() {
    static TickManager inst;
    return inst;
  }

  bool is_initialized() const { return m_is_initialized; }
  void initialize();

  void sleep(uint64_t ms);
  void increment_ticks();
  uint64_t get_ticks() { return m_ticks; }
  uint32_t get_frequency() const { return m_frequency; }
  void set_frequency(uint32_t frequency) { m_frequency = frequency; }
};

} // namespace fkernel
using fkernel::TickManager;
