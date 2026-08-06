#include <LibFK/Algorithms/Logging/log.h>

#include <Kernel/Arch/x86_64/Interrupt/Handler/handlers.h>
#include <Kernel/Arch/x86_64/Interrupt/Handler/interrupt_frame.h>
#include <Kernel/Arch/x86_64/Interrupt/HardwareInterrupts/hardware_interrupt_manager.h>
#include <Kernel/Arch/x86_64/Interrupt/HardwareInterrupts/tick_manager.h>
#include <Kernel/Arch/x86_64/Interrupt/interrupt_controller.h>
#include <Kernel/Arch/x86_64/Syscall/syscall_arch.h>
#include <Kernel/Ipc/Signals/signal_delivery.h>
#include <Kernel/Scheduler/Core/scheduler.h>

static_assert(sizeof(InterruptFrame) == 22 * sizeof(uint64_t),
              "InterruptFrame layout changed — update PtRegs bridge copy");
static_assert(sizeof(PtRegs) == 16 * sizeof(uint64_t),
              "PtRegs layout changed — update InterruptFrame bridge copy");
static_assert(__builtin_offsetof(InterruptFrame, rip)    == 17 * sizeof(uint64_t));
static_assert(__builtin_offsetof(InterruptFrame, rflags) == 19 * sizeof(uint64_t));
static_assert(__builtin_offsetof(InterruptFrame, rsp)    == 20 * sizeof(uint64_t));
static_assert(__builtin_offsetof(PtRegs, rip)    == 13 * sizeof(uint64_t));
static_assert(__builtin_offsetof(PtRegs, rflags) == 14 * sizeof(uint64_t));
static_assert(__builtin_offsetof(PtRegs, rsp)    == 15 * sizeof(uint64_t));

// TSC latency stats (sprint Item 1): max handler cycles per interrupt vector.
// Not locked — a lost update on a concurrent per-CPU update is acceptable for a max tracker.
// g_tsc_max_syscall is defined in syscall.cpp (extern here for the periodic dump).
extern uint64_t g_tsc_max_syscall;
static uint64_t g_tsc_max_irq[256] = {};
static uint64_t g_tsc_last_dump_ticks = 0;
static constexpr uint64_t TSC_DUMP_INTERVAL_MS = 5000;

static inline uint64_t irq_tsc_now() {
    uint32_t lo, hi;
    asm volatile("rdtsc" : "=a"(lo), "=d"(hi));
    return (static_cast<uint64_t>(hi) << 32) | lo;
}

static void tsc_latency_dump(uint64_t now_ticks) {
    g_tsc_last_dump_ticks = now_ticks;
    bool any = false;
    for (size_t v = 0; v < 256; ++v) {
        if (!g_tsc_max_irq[v]) continue;
        if (!any) { fk::algorithms::klog("TSC", "Max handler latencies (5s window):"); any = true; }
        fk::algorithms::klog("TSC", "  [%s] vec %3zu: %lu cy",
            v < 32 ? "EXC" : "IRQ", v, g_tsc_max_irq[v]);
        g_tsc_max_irq[v] = 0;
    }
    if (g_tsc_max_syscall) {
        if (!any) fk::algorithms::klog("TSC", "Max handler latencies (5s window):");
        fk::algorithms::klog("TSC", "  [SYS] syscall:     %lu cy", g_tsc_max_syscall);
        g_tsc_max_syscall = 0;
    }
}

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

  if (now_ticks - g_tsc_last_dump_ticks >= TSC_DUMP_INTERVAL_MS)
    tsc_latency_dump(now_ticks);

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

  uint64_t tsc_start = irq_tsc_now();
  auto handler = InterruptController::the().get_interrupt(vector);
  fk::algorithms::ktrace("IRQ", "dispatch vec=%u handler=%s", (unsigned)vector, handler ? "registered" : "default");
  if (handler)
    handler(vector, frame);
  else
    default_handler(vector, frame);
  uint64_t tsc_elapsed = irq_tsc_now() - tsc_start;
  if (tsc_elapsed > g_tsc_max_irq[vector])
    g_tsc_max_irq[vector] = tsc_elapsed;

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
