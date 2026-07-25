#include <Kernel/Arch/x86_64/Interrupt/Handler/handlers.h>
#include <Kernel/Arch/x86_64/Interrupt/Handler/interrupt_frame.h>
#include <Kernel/Arch/x86_64/Interrupt/interrupt_controller.h>
#include <Kernel/Arch/x86_64/Syscall/syscall_arch.h>
#include <Kernel/Ipc/signal_delivery.h>
#include <Kernel/Scheduler/scheduler.h>

extern "C" void interrupt_dispatch(uint8_t vector,
                                   InterruptFrame *frame = nullptr) {
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
