#include <Kernel/Arch/x86_64/Interrupt/Handler/handlers.h>
#include <Kernel/Arch/x86_64/Interrupt/Handler/interrupt_frame.h>
#include <Kernel/Arch/x86_64/Interrupt/HardwareInterrupts/hardware_interrupt_manager.h>
#include <Kernel/Arch/x86_64/Interrupt/HardwareInterrupts/tick_manager.h>
#include <Kernel/Arch/x86_64/Interrupt/interrupt_controller.h>
#include <Kernel/Arch/x86_64/Syscall/syscall_arch.h>
#include <Kernel/Ipc/Signals/signal_delivery.h>
#include <Kernel/Scheduler/Core/scheduler.h>
#include <LibFK/Algorithms/Logging/log.h>

namespace {

// IRQ storm detection: count interrupts per vector over a fixed time window. A device
// stuck asserting (un-EOI'd) livelocks the CPU at millions of interrupts/sec; a legit
// timer at 1000 Hz is only ~100 interrupts per window. Masking the offending IRQ turns
// the livelock into a logged, degraded-but-alive system.
constexpr size_t STORM_SLOTS = 256;
constexpr uint64_t STORM_WINDOW_MS = 100;
constexpr uint64_t STORM_MAX_PER_WINDOW = 2000; // 20k/s ceiling per vector

uint64_t g_storm_counts[STORM_SLOTS] = {0};
uint64_t g_storm_last_ticks = 0;
bool g_storm_masked[STORM_SLOTS] = {false};

void evaluate_irq_storms() {
  uint64_t now_ticks = TickManager::the().get_ticks();
  if (now_ticks == g_storm_last_ticks) return; // window not elapsed yet
  if (g_storm_last_ticks == 0) {               // first ticked sample: baseline only
    g_storm_last_ticks = now_ticks;
    for (size_t v = 0; v < STORM_SLOTS; ++v)
      g_storm_counts[v] = 0;
    return;
  }
  if (now_ticks - g_storm_last_ticks < STORM_WINDOW_MS) return;
  g_storm_last_ticks = now_ticks;

  for (size_t v = 0; v < STORM_SLOTS; ++v) {
    if (g_storm_counts[v] >= STORM_MAX_PER_WINDOW) {
      if (!g_storm_masked[v]) {
        g_storm_masked[v] = true;
        if (v >= 32) {
          uint8_t irq = static_cast<uint8_t>(v - 32);
          fk::algorithms::kwarn("IRQ_STORM",
              "Storm on vector %u (IRQ %u): %lu in %llu ms. Masking IRQ %u.",
              (unsigned)v, (unsigned)irq, (unsigned long)g_storm_counts[v],
              (unsigned long long)STORM_WINDOW_MS, (unsigned)irq);
          HardwareInterruptManager::the().mask_interrupt(irq);
        } else {
          fk::algorithms::kwarn("IRQ_STORM",
              "Exception vector %u storm: %lu in %llu ms. Cannot mask.",
              (unsigned)v, (unsigned long)g_storm_counts[v],
              (unsigned long long)STORM_WINDOW_MS);
        }
      }
    } else {
      g_storm_masked[v] = false;
    }
    g_storm_counts[v] = 0;
  }
}

} // namespace

extern "C" void interrupt_dispatch(uint8_t vector,
                                   InterruptFrame *frame = nullptr) {
  ++g_storm_counts[vector];
  evaluate_irq_storms();

  auto handler = InterruptController::the().get_interrupt(vector);
  if (handler) {
    handler(vector, frame);
  } else
    default_handler(vector, frame);

  if (SchedulerManager::the().is_initialized() &&
      SchedulerManager::the().is_need_resched()) {
    SchedulerManager::the().schedule();
  }

  // Deliver pending signals when returning to user mode.
  // Convert the InterruptFrame to PtRegs, let install_handler_frame redirect
  // rip/rsp to the signal handler, then write the changes back so iretq
  // returns to the handler instead of the original interrupted context.
  if (frame && (frame->cs & 3) == 3) {
    auto* task = SchedulerManager::the().current();
    if (task && !task->is_a_kernel_task() && task->has_pending_signals()) {
      PtRegs regs{};
      regs.r9      = frame->r9;
      regs.r8      = frame->r8;
      regs.r10     = frame->r10;
      regs.rdx     = frame->rdx;
      regs.rsi     = frame->rsi;
      regs.rdi     = frame->rdi;
      regs.rax     = frame->rax;
      regs.r15     = frame->r15;
      regs.r14     = frame->r14;
      regs.r13     = frame->r13;
      regs.r12     = frame->r12;
      regs.rbp     = frame->rbp;
      regs.rbx     = frame->rbx;
      regs.rip     = frame->rip;
      regs.rflags  = frame->rflags;
      regs.rsp     = frame->rsp;

      fkernel::ipc::SignalDelivery::handle_pending_signals(task, &regs);

      // Propagate any redirections (signal handler installed) back to the
      // interrupt frame so iretq returns to the signal handler.
      frame->rip    = regs.rip;
      frame->rflags = regs.rflags;
      frame->rsp    = regs.rsp;
      frame->rdi    = regs.rdi;
    }
  }
}
