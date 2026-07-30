#pragma once

#include <LibFK/Core/result.h>
#include <LibFK/Text/string.h>
#include <LibFK/Types/types.h>

#include <Kernel/Clock/Types/datetime.h>
#include <Kernel/Clock/Types/clock.h>

class ClockManager {
private:
  Clock *m_clock = nullptr;
  bool m_initialized = false;

  ClockManager() = default;

public:
  static ClockManager &the() {
    static ClockManager inst;
    return inst;
  }

  bool is_initialized() const { return m_initialized; }

  void initialize();

  void set_clock(Clock *clock);
  DateTime datetime();

  void set_memory_manager(bool has_memory_manager) {
    // Currently no clocks depend on memory manager, but this is for future
    // proofing
    (void)has_memory_manager;
  }

private:
  void select_and_configure_clock();
};
