#pragma once

#include <Kernel/Arch/x86_64/Interrupt/HardwareInterrupts/TickManager.h>
#include <LibFK/Types/types.h>

class Timer {
public:
  virtual void initialize(uint32_t frequency) = 0;
  virtual ~Timer() = default;
};

class TimerManager {
private:
  Timer *m_timer = nullptr;
  TickManager m_tick;
  bool m_has_memory_manager = false;

public:
  TimerManager() = default;

  static TimerManager &the() {
    static TimerManager inst;
    return inst;
  }

  void initialize(uint32_t freq);
  void sleep(uint64_t awaited_ticks);

  void set_memory_manager(bool has_memory_manager) {
    m_has_memory_manager = has_memory_manager;
  }
};
