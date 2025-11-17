#pragma once

#include "LibFK/Container/string.h"
#include <LibFK/Core/Result.h>
#include <LibFK/Types/types.h>

struct DateTime {
  uint8_t second;
  uint8_t minute;
  uint8_t hour;
  uint8_t day;
  uint8_t month;
  uint16_t year;

  String to_string() const;
  void print();
};

class Clock {
public:
  virtual ~Clock() = default;

  virtual fk::core::Result<void> initialize(uint32_t frequency) = 0;
  virtual String get_name() = 0;
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
};
