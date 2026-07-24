#pragma once

#include <LibFK/Types/types.h>

class BootTimer {
public:
  static constexpr size_t MAX_MARKS = 32;

  struct Mark {
    const char* name;
    uint64_t tsc;
  };

private:
  Mark m_marks[MAX_MARKS];
  size_t m_count = 0;

  BootTimer() = default;

public:
  static BootTimer& the() {
    static BootTimer inst;
    return inst;
  }

  void mark(const char* name);
  void log_summary();
};
