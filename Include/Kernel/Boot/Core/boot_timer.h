#pragma once

#include <Kernel/Boot/Core/boot_mark.h>
#include <LibFK/Types/types.h>

class BootTimer {
public:
  static constexpr size_t MAX_MARKS = 32;

private:
  BootMark m_marks[MAX_MARKS];
  size_t m_count = 0;

  BootTimer() = default;

public:
  static BootTimer& the() {
    static BootTimer inst;
    return inst;
  }

  void mark(const char* name);
  void log_summary();
  void set_tsc_frequency(uint64_t freq_hz);
};
