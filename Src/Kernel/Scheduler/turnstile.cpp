#include <Kernel/Scheduler/turnstile.h>
#include <Kernel/Scheduler/Task/task.h>
#include <Kernel/Scheduler/qos.h>
#include <LibFK/Algorithms/log.h>

namespace fkernel::scheduler {

Turnstile* create_turnstile(::Task* holder, ::Task* waiter) {
    auto* ts = new Turnstile();
    ts->holder = holder;
    ts->waiter = waiter;
    ts->original_qos = holder->control.lifecycle.qos;
    ts->boosted_qos = waiter->control.lifecycle.qos;
    ts->active = false;
    ts->chain = nullptr;
    return ts;
}

void destroy_turnstile(Turnstile* ts) {
    if (!ts) return;
    if (ts->chain) destroy_turnstile(ts->chain);
    delete ts;
}

void boost_qos_if_needed(::Task* waiter, ::Task* holder) {
    if (!waiter || !holder) return;

    QoSClass waiter_qos = waiter->control.lifecycle.qos;
    QoSClass holder_qos = holder->control.lifecycle.qos;

    if (static_cast<uint8_t>(waiter_qos) >= static_cast<uint8_t>(holder_qos))
        return;

    if (holder->control.lifecycle.boosted)
        return;

    holder->control.lifecycle.boosted = true;
    holder->control.lifecycle.original_qos = holder_qos;
    holder->control.lifecycle.qos = waiter_qos;

    reprioritize_task(holder);

    fk::algorithms::kdebug("TURNSTILE", "Task %lu boosted from QoS %d to %d",
        holder->control.identity.id.value(),
        static_cast<uint8_t>(holder_qos),
        static_cast<uint8_t>(waiter_qos));
}

void unboost_task(::Task* task) {
    if (!task) return;
    if (!task->control.lifecycle.boosted) return;

    QoSClass original = task->control.lifecycle.original_qos;
    task->control.lifecycle.qos = original;
    task->control.lifecycle.boosted = false;

    reprioritize_task(task);

    fk::algorithms::kdebug("TURNSTILE", "Task %lu unboosted to QoS %d",
        task->control.identity.id.value(),
        static_cast<uint8_t>(original));
}

void reprioritize_task(::Task* task) {
    if (!task) return;
    task->control.lifecycle.base_priority =
        priority_for_qos(task->control.lifecycle.qos, task->control.lifecycle.nice);
    task->control.lifecycle.priority = task->control.lifecycle.base_priority;
}

} // namespace fkernel::scheduler
