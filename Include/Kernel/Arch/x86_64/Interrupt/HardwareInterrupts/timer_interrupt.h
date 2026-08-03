#pragma once

#include <Kernel/Arch/x86_64/Interrupt/HardwareInterrupts/tick_manager.h>
#include <Kernel/Arch/x86_64/Interrupt/HardwareInterrupts/timer.h>
#include <LibFK/Types/types.h>

namespace fkernel {

class TimerManager {
private:
  Timer *m_timer = nullptr;
  bool m_has_memory_manager = false;
  bool m_is_initialized{false};

  TimerManager() = default;
  TimerManager(const TimerManager &) = delete;
  TimerManager &operator=(const TimerManager &) = delete;

public:
  static TimerManager &the() {
    static TimerManager inst;
    return inst;
  }

  bool is_initialized() const { return m_is_initialized; }

  void initialize(uint32_t freq = 1000);
  void sleep(uint64_t awaited_ticks);

  void set_timer(Timer *timer);
  void set_memory_manager(bool has_memory_manager);

private:
  void select_and_configure_timer(uint32_t freq);
};

} // namespace fkernel
using fkernel::Timer;
using fkernel::TimerManager;
