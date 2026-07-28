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

    if (holder->control.lifecycle.boosted) {
        // Holder is already boosted — walk the chain to propagate transitively
        Turnstile* chain = holder->resources.ipc.active_turnstile;
        if (!chain) return;

        // Find the end of the chain
        size_t depth = 0;
        Turnstile* tail = chain;
        while (tail->chain) {
            tail = tail->chain;
            if (++depth >= MAX_CHAIN_DEPTH) return;
        }

        // If waiter's QoS is higher priority than current boost target, re-boost
        if (static_cast<uint8_t>(waiter_qos) >= static_cast<uint8_t>(tail->boosted_qos))
            return;

        holder->control.lifecycle.qos = waiter_qos;
        auto* ts = new Turnstile();
        ts->holder = holder;
        ts->waiter = waiter;
        ts->original_qos = holder_qos;
        ts->boosted_qos = waiter_qos;
        ts->active = true;
        ts->chain = nullptr;
        tail->chain = ts;

        reprioritize_task(holder);
        fk::algorithms::kdebug("TURNSTILE", "Task %lu chain-boosted from QoS %d to %d (depth %zu)",
            holder->control.identity.id.value(),
            static_cast<uint8_t>(holder_qos),
            static_cast<uint8_t>(waiter_qos), depth + 1);
        return;
    }

    holder->control.lifecycle.boosted = true;
    holder->control.lifecycle.original_qos = holder_qos;
    holder->control.lifecycle.qos = waiter_qos;

    auto* ts = new Turnstile();
    ts->holder = holder;
    ts->waiter = waiter;
    ts->original_qos = holder_qos;
    ts->boosted_qos = waiter_qos;
    ts->active = true;
    ts->chain = nullptr;
    holder->resources.ipc.active_turnstile = ts;

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

    // Walk and destroy the chain
    Turnstile* chain = task->resources.ipc.active_turnstile;
    while (chain) {
        Turnstile* next = chain->chain;
        if (chain->holder && chain->holder != task) {
            chain->holder->control.lifecycle.qos = chain->holder->control.lifecycle.original_qos;
            chain->holder->control.lifecycle.boosted = false;
            chain->holder->resources.ipc.active_turnstile = nullptr;
            reprioritize_task(chain->holder);
        }
        delete chain;
        chain = next;
    }
    task->resources.ipc.active_turnstile = nullptr;

    reprioritize_task(task);

    fk::algorithms::kdebug("TURNSTILE", "Task %lu unboosted to QoS %d",
        task->control.identity.id.value(),
        static_cast<uint8_t>(original));
}

void reprioritize_task(::Task* task) {
    if (!task) return;
    task->control.lifecycle.base_priority =
        priority_for_qos(task->control.lifecycle.qos, fk::NiceValue(task->control.lifecycle.nice)).value();
    task->control.lifecycle.priority = task->control.lifecycle.base_priority;
}

} // namespace fkernel::scheduler
