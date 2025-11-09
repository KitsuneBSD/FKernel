#pragma once

#include <LibFK/Types/types.h>

class Timer {
public:
  virtual void initialize(uint32_t frequency) = 0;
  virtual void increment_ticks() = 0;
  virtual uint64_t get_ticks() = 0;
  virtual void sleep(uint64_t ms) = 0;
  virtual ~Timer() = default;
};

class TimerManager {
private:
  Timer *m_timer = nullptr;
  bool m_has_memory_manager = false;

public:
  TimerManager() = default;

  static TimerManager &the() {
    static TimerManager inst;
    return inst;
  }

  void initialize(uint32_t freq);
  void sleep(uint64_t ms);

  void increment_ticks();
  void set_memory_manager(bool has_memory_manager) {
    m_has_memory_manager = has_memory_manager;
  }

  uint64_t get_ticks() { return m_timer ? m_timer->get_ticks() : 0; }
};
