#pragma once

#include <LibFK/Core/result.h>
#include <LibFK/Text/string.h>
#include <LibFK/Types/types.h>

#include <Kernel/Clock/Types/datetime.h>
#include <Kernel/Clock/Types/clock.h>

namespace fkernel {

class ClockManager {
  Clock* m_clock = nullptr;
  bool m_is_initialized{false};

  ClockManager() = default;
  ClockManager(const ClockManager&) = delete;
  ClockManager& operator=(const ClockManager&) = delete;
  ClockManager(ClockManager&&) = delete;
  ClockManager& operator=(ClockManager&&) = delete;

  void select_and_configure_clock();

public:
  static ClockManager& the() {
    static ClockManager inst;
    return inst;
  }

  bool is_initialized() const { return m_is_initialized; }

  void initialize();

  void set_clock(Clock* clock);
  DateTime datetime();

  void set_memory_manager(bool has_memory_manager) {
    (void)has_memory_manager;
  }
};

} // namespace fkernel
using fkernel::ClockManager;
