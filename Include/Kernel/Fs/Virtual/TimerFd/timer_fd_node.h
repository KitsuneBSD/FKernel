#pragma once

#include <Kernel/Fs/Vfs/node.h>
#include <Kernel/Ipc/endpoint.h>
#include <LibFK/Core/result.h>
#include <LibFK/Synchronization/spinlock.h>
#include <LibFK/Types/types.h>

namespace fkernel {

class TimerFdNode final : public Node {
public:
    struct KernelTimespec {
        int64_t tv_sec;
        int64_t tv_nsec;
    };

    struct KernelItimerspec {
        KernelTimespec it_interval;
        KernelTimespec it_value;
    };

    static fk::core::Result<fk::RefPtr<TimerFdNode>, fk::core::Error> create(int clock_id);

    explicit TimerFdNode(int clock_id);
    virtual ~TimerFdNode() override = default;

    virtual fk::core::Result<size_t, fk::core::Error> read(
        uint64_t offset, size_t size, uint8_t* buffer) override;
    virtual fk::core::Result<size_t, fk::core::Error> write(
        [[maybe_unused]] uint64_t offset,
        [[maybe_unused]] size_t size,
        [[maybe_unused]] const uint8_t* buffer) override {
        return fk::core::Error::NotImplemented;
    }
    virtual size_t size() const override { return sizeof(uint64_t); }
    virtual bool is_timerfd() const override { return true; }
    virtual short poll() const override;

    void settime(const KernelItimerspec& new_spec, KernelItimerspec* old_spec);
    void gettime(KernelItimerspec& out_spec) const;
    void on_tick(uint64_t now_ticks, uint32_t frequency);
    void set_nonblock(bool v) { m_nonblock = v; }

private:
    static uint64_t timespec_to_ticks(const KernelTimespec& ts, uint32_t frequency);
    void rearm(uint32_t frequency);

    mutable fk::synchronization::Spinlock m_lock;
    int m_clock_id;
    KernelItimerspec m_spec{};
    uint64_t m_expirations{0};
    uint64_t m_next_tick{0};
    bool m_armed{false};
    bool m_nonblock{false};
    ipc::Endpoint m_endpoint;
};

} // namespace fkernel
