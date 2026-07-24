#include <Kernel/Fs/TimerFd/timer_fd_registry.h>
#include <Kernel/Fs/TimerFd/timer_fd_node.h>
#include <LibFK/Container/vector.h>
#include <LibFK/Synchronization/spinlock.h>

namespace fkernel {
namespace timer_fd_registry {

namespace {
    fk::synchronization::Spinlock s_lock;
    fk::containers::Vector<TimerFdNode*> s_timers;
}

void register_timer(TimerFdNode* node) {
    fk::synchronization::ScopedLock lock(s_lock);
    s_timers.push_back(node);
}

void unregister_timer(TimerFdNode* node) {
    fk::synchronization::ScopedLock lock(s_lock);
    for (size_t i = 0; i < s_timers.size(); ++i) {
        if (s_timers[i] == node) {
            s_timers[i] = s_timers[s_timers.size() - 1];
            s_timers.pop_back();
            return;
        }
    }
}

void tick_all(uint64_t now_ticks, uint32_t frequency) {
    fk::synchronization::ScopedLock lock(s_lock);
    for (size_t i = 0; i < s_timers.size(); ++i)
        s_timers[i]->on_tick(now_ticks, frequency);
}

} // namespace timer_fd_registry
} // namespace fkernel
