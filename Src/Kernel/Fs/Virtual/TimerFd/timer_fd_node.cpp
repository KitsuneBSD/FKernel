#include <Kernel/Fs/Virtual/TimerFd/timer_fd_node.h>
#include <Kernel/Fs/Virtual/TimerFd/timer_fd_registry.h>
#include <Kernel/Arch/x86_64/Interrupt/HardwareInterrupts/tick_manager.h>
#include <LibFK/Memory/ref_ptr.h>

namespace fkernel {

fk::core::Result<fk::RefPtr<TimerFdNode>, fk::core::Error>
TimerFdNode::create(int clock_id) {
    return fk::RefPtr<TimerFdNode>(new TimerFdNode(clock_id));
}

TimerFdNode::TimerFdNode(int clock_id) : m_clock_id(clock_id) {}

uint64_t TimerFdNode::timespec_to_ticks(const KernelTimespec& ts, uint32_t frequency) {
    if (frequency == 0)
        return 0;
    return static_cast<uint64_t>(ts.tv_sec) * frequency
         + static_cast<uint64_t>(ts.tv_nsec) * frequency / 1000000000ULL;
}

void TimerFdNode::settime(const KernelItimerspec& new_spec, KernelItimerspec* old_spec) {
    uint32_t frequency = TickManager::the().get_frequency();
    uint64_t now = TickManager::the().get_ticks();

    m_lock.lock();

    if (old_spec)
        *old_spec = m_spec;

    bool was_armed = m_armed;
    if (was_armed)
        m_lock.unlock();
    if (was_armed)
        timer_fd_registry::unregister_timer(this);
    if (was_armed)
        m_lock.lock();

    m_spec = new_spec;
    m_armed = false;
    m_expirations = 0;

    uint64_t initial_ticks = timespec_to_ticks(new_spec.it_value, frequency);
    if (initial_ticks > 0) {
        m_next_tick = now + initial_ticks;
        m_armed = true;
        m_lock.unlock();
        timer_fd_registry::register_timer(this);
        return;
    }

    m_lock.unlock();
}

void TimerFdNode::gettime(KernelItimerspec& out_spec) const {
    uint32_t frequency = TickManager::the().get_frequency();
    uint64_t now = TickManager::the().get_ticks();

    m_lock.lock();
    out_spec = m_spec;
    if (m_armed && m_next_tick > now && frequency > 0) {
        uint64_t remaining_ticks = m_next_tick - now;
        out_spec.it_value.tv_sec  = static_cast<int64_t>(remaining_ticks / frequency);
        out_spec.it_value.tv_nsec = static_cast<int64_t>(
            (remaining_ticks % frequency) * 1000000000ULL / frequency);
    } else if (!m_armed) {
        out_spec.it_value = {0, 0};
    }
    m_lock.unlock();
}

void TimerFdNode::on_tick(uint64_t now_ticks, uint32_t frequency) {
    m_lock.lock();
    if (!m_armed || now_ticks < m_next_tick) {
        m_lock.unlock();
        return;
    }

    ++m_expirations;

    uint64_t interval_ticks = timespec_to_ticks(m_spec.it_interval, frequency);
    if (interval_ticks > 0) {
        m_next_tick += interval_ticks;
    } else {
        m_armed = false;
    }
    m_lock.unlock();

    m_readable.signal(1);
}

fk::core::Result<size_t, fk::core::Error>
TimerFdNode::read(uint64_t, size_t size, uint8_t* buffer) {
    if (size < sizeof(uint64_t))
        return fk::core::Error::InvalidParameter;

    m_lock.lock();
    while (m_expirations == 0) {
        m_lock.unlock();
        if (m_nonblock)
            return fk::core::Error::WouldBlock;
        m_readable.wait();
        m_lock.lock();
    }
    uint64_t val = m_expirations;
    m_expirations = 0;
    m_lock.unlock();

    __builtin_memcpy(buffer, &val, sizeof(uint64_t));
    return sizeof(uint64_t);
}

short TimerFdNode::poll() const {
    m_lock.lock();
    uint64_t exp = m_expirations;
    m_lock.unlock();
    return (exp > 0) ? (POLLIN | POLLOUT) : POLLOUT;
}

} // namespace fkernel
