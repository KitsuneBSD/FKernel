#include <LibFK/Algorithms/Logging/log.h>

#include <Kernel/Scheduler/Sync/turnstile.h>
#include <Kernel/Scheduler/Task/task.h>
#include <Kernel/Scheduler/Qos/qos.h>

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

// Extend an already-boosted holder's chain with waiter's QoS.
// Returns false if no extension was needed or chain is full.
static bool extend_chain(::Task* waiter, ::Task* holder, QoSClass waiter_qos) {
    Turnstile* chain = holder->resources.ipc.active_turnstile;
    if (!chain) return false;

    size_t depth = 0;
    Turnstile* tail = chain;
    while (tail->chain) {
        tail = tail->chain;
        if (++depth >= MAX_CHAIN_DEPTH) return false;
    }

    if (static_cast<uint8_t>(waiter_qos) >= static_cast<uint8_t>(tail->boosted_qos))
        return false;

    auto* ts = new Turnstile();
    ts->holder  = holder;
    ts->waiter  = waiter;
    ts->original_qos = holder->control.lifecycle.qos;
    ts->boosted_qos  = waiter_qos;
    ts->active  = true;
    ts->chain   = nullptr;
    tail->chain = ts;
    waiter->resources.ipc.pending_turnstile = ts;
    return true;
}

// Walk holder->pending_turnstile chain and propagate waiter_qos transitively.
static void propagate_transitive(::Task* holder, QoSClass waiter_qos) {
    ::Task* current = holder;
    for (size_t depth = 0; depth < MAX_CHAIN_DEPTH; ++depth) {
        Turnstile* pending = current->resources.ipc.pending_turnstile;
        if (!pending || !pending->holder) break;
        ::Task* next_holder = pending->holder;

        if (static_cast<uint8_t>(waiter_qos) >= static_cast<uint8_t>(next_holder->control.lifecycle.qos))
            break;

        if (!next_holder->control.lifecycle.boosted) {
            next_holder->control.lifecycle.boosted    = true;
            next_holder->control.lifecycle.original_qos = next_holder->control.lifecycle.qos;
        }
        next_holder->control.lifecycle.qos = waiter_qos;
        reprioritize_task(next_holder);
        fk::algorithms::kdebug("TURNSTILE", "Task %lu transitively boosted to QoS %d (depth %zu)",
            next_holder->control.identity.id.value(),
            static_cast<uint8_t>(waiter_qos), depth + 1);

        current = next_holder;
    }
}

// Remove the turnstile where `waiter` is the waiter from `holder`'s active chain.
// Unboosts holder if the chain becomes empty.
static void remove_from_holder_chain(::Task* waiter, ::Task* holder) {
    Turnstile* prev = nullptr;
    Turnstile* ts   = holder->resources.ipc.active_turnstile;
    while (ts) {
        if (ts->waiter == waiter) {
            if (prev) prev->chain = ts->chain;
            else      holder->resources.ipc.active_turnstile = ts->chain;
            delete ts;
            break;
        }
        prev = ts;
        ts   = ts->chain;
    }

    if (!holder->resources.ipc.active_turnstile) {
        holder->control.lifecycle.qos     = holder->control.lifecycle.original_qos;
        holder->control.lifecycle.boosted = false;
        reprioritize_task(holder);
        fk::algorithms::kdebug("TURNSTILE", "Task %lu transitively unboosted",
            holder->control.identity.id.value());
    }
}

void boost_qos_if_needed(::Task* waiter, ::Task* holder) {
    if (!waiter || !holder) return;

    QoSClass waiter_qos = waiter->control.lifecycle.qos;
    QoSClass holder_qos = holder->control.lifecycle.qos;

    if (static_cast<uint8_t>(waiter_qos) >= static_cast<uint8_t>(holder_qos))
        return;

    if (holder->control.lifecycle.boosted) {
        if (!extend_chain(waiter, holder, waiter_qos)) return;
        holder->control.lifecycle.qos = waiter_qos;
        reprioritize_task(holder);
        fk::algorithms::kdebug("TURNSTILE", "Task %lu chain-boosted to QoS %d",
            holder->control.identity.id.value(),
            static_cast<uint8_t>(waiter_qos));
    } else {
        holder->control.lifecycle.boosted      = true;
        holder->control.lifecycle.original_qos = holder_qos;
        holder->control.lifecycle.qos          = waiter_qos;

        auto* ts        = new Turnstile();
        ts->holder      = holder;
        ts->waiter      = waiter;
        ts->original_qos = holder_qos;
        ts->boosted_qos  = waiter_qos;
        ts->active      = true;
        ts->chain       = nullptr;
        holder->resources.ipc.active_turnstile  = ts;
        waiter->resources.ipc.pending_turnstile = ts;

        reprioritize_task(holder);
        fk::algorithms::kdebug("TURNSTILE", "Task %lu boosted QoS %d -> %d",
            holder->control.identity.id.value(),
            static_cast<uint8_t>(holder_qos),
            static_cast<uint8_t>(waiter_qos));
    }

    // Phase 35c: transitive PI — propagate boost to whoever holder is waiting on
    propagate_transitive(holder, waiter_qos);
}

void unboost_task(::Task* task) {
    if (!task) return;
    if (!task->control.lifecycle.boosted) return;

    QoSClass original = task->control.lifecycle.original_qos;
    task->control.lifecycle.qos     = original;
    task->control.lifecycle.boosted = false;

    // Walk and destroy this task's active turnstile chain
    Turnstile* chain = task->resources.ipc.active_turnstile;
    while (chain) {
        Turnstile* next = chain->chain;
        // Clear the waiter's pending_turnstile to avoid dangling pointer
        if (chain->waiter)
            chain->waiter->resources.ipc.pending_turnstile = nullptr;
        if (chain->holder && chain->holder != task) {
            chain->holder->control.lifecycle.qos     = chain->holder->control.lifecycle.original_qos;
            chain->holder->control.lifecycle.boosted = false;
            chain->holder->resources.ipc.active_turnstile = nullptr;
            reprioritize_task(chain->holder);
        }
        delete chain;
        chain = next;
    }
    task->resources.ipc.active_turnstile = nullptr;

    // Phase 35c: if this task was itself waiting on another holder,
    // remove it from that holder's chain and possibly unboost the holder.
    if (task->resources.ipc.pending_turnstile) {
        ::Task* pending_holder = task->resources.ipc.pending_turnstile->holder;
        if (pending_holder)
            remove_from_holder_chain(task, pending_holder);
        task->resources.ipc.pending_turnstile = nullptr;
    }

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
