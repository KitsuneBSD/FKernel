#pragma once

#include <LibFK/Text/string.h>
#include <LibFK/Core/Result.h>
#include <LibFK/Types/types.h>

struct DateTime {
  uint8_t second;
  uint8_t minute;
  uint8_t hour;
  uint8_t day;
  uint8_t month;
  uint16_t year;

  fk::text::String to_string() const;
  void print();
};

class Clock {
public:
  virtual ~Clock() = default;

  virtual fk::core::Result<void> initialize(uint32_t frequency) = 0;
  virtual fk::text::String get_name() = 0;
  virtual DateTime datetime() { return {}; }
};

class ClockManager {
private:
  Clock *m_clock = nullptr;

public:
  ClockManager() = default;

  static ClockManager &the() {
    static ClockManager inst;
    return inst;
  }

  void initialize();

  void set_clock(Clock *clock);
  DateTime datetime();

  void set_memory_manager(bool has_memory_manager) { 
    // Currently no clocks depend on memory manager, but this is for future proofing
    (void)has_memory_manager; 
  }

private:
  void select_and_configure_clock();
};
