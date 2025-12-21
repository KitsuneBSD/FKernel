#include <Kernel/Arch/x86_64/Interrupt/Handler/interrupt_frame.h>
#include <Kernel/Arch/x86_64/Interrupt/HardwareInterrupts/hardware_interrupt.h>
#include <Kernel/Arch/x86_64/Interrupt/HardwareInterrupts/tick_manager.h>
#include <Kernel/Arch/x86_64/Interrupt/HardwareInterrupts/timer_interrupt.h>
#include <Kernel/Arch/x86_64/io.h>
#include <Kernel/Scheduler/scheduler.h>
#include <LibFK/Algorithms/log.h>


// TODO: This is a bullshit made on a hurry. Refactor the scheduler and task to remove this faster.
static inline bool is_real_task(Task* task) {
    return task && !task->is_idle();
}

static inline void save_context(Task* task, InterruptFrame* frame) {
    task->context.rip    = frame->rip;
    task->context.rflags = frame->rflags;
}

static inline void load_context(Task* task, InterruptFrame* frame) {
    frame->rip    = task->context.rip;
    frame->rflags = task->context.rflags;
}

void scheduler_update(InterruptFrame* frame) {
    auto& sched = SchedulerManager::the();
    Task* current = sched.current();

    if (is_real_task(current)) {
        save_context(current, frame);
    }

    sched.on_tick();

    if (!sched.need_resched())
        return;

    Task* next = sched.pick_next();
    if (!next)
        return;

    sched.clear_resched();

    if (is_real_task(next)) {
        load_context(next, frame);
    }
}


void timer_handler([[maybe_unused]] uint8_t vector, InterruptFrame *frame) {
  TickManager::the().increment_ticks();
  scheduler_update(frame);
  
  HardwareInterruptManager::the().send_eoi(vector);
}
