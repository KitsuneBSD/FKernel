#include <Kernel/Arch/x86_64/Interrupt/Handler/handlers.h>
#include <Kernel/Arch/x86_64/Interrupt/Handler/interrupt_frame.h>
#include <Kernel/Arch/x86_64/Interrupt/interrupt_controller.h>
#include <Kernel/Scheduler/scheduler.h>

extern "C" void interrupt_dispatch(uint8_t vector,
                                   InterruptFrame *frame = nullptr) {
  auto handler = InterruptController::the().get_interrupt(vector);
  if (handler) {
    handler(vector, frame);
  } else
    default_handler(vector, frame);

  // Only schedule if explicitly requested (e.g., by timer interrupt)
  // This prevents double-scheduling and excessive context switching
  if (SchedulerManager::the().is_need_resched()) {
    SchedulerManager::the().schedule();
  }
}
