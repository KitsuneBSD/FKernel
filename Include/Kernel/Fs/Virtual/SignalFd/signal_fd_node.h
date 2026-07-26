#pragma once

#include <Kernel/Fs/Vfs/node.h>
#include <Kernel/Ipc/notification.h>
#include <LibFK/Container/vector.h>
#include <LibFK/Core/result.h>
#include <LibFK/Synchronization/spinlock.h>
#include <LibFK/Types/types.h>

struct Task;

namespace fkernel {

struct SignalfdSiginfo {
    uint32_t ssi_signo;
    int32_t  ssi_errno;
    int32_t  ssi_code;
    uint32_t ssi_pid;
    uint32_t ssi_uid;
    int32_t  ssi_fd;
    uint32_t ssi_tid;
    uint32_t ssi_band;
    uint32_t ssi_overrun;
    uint32_t ssi_trapno;
    int32_t  ssi_status;
    int32_t  ssi_int;
    uint64_t ssi_ptr;
    uint64_t ssi_utime;
    uint64_t ssi_stime;
    uint64_t ssi_addr;
    uint8_t  _pad[48];
};

class SignalFdNode final : public Node {
public:
    static fk::core::Result<fk::RefPtr<SignalFdNode>, fk::core::Error> create(
        Task* task, uint64_t mask);

    SignalFdNode(Task* task, uint64_t mask);
    virtual ~SignalFdNode() override = default;

    virtual fk::core::Result<size_t, fk::core::Error> read(
        uint64_t offset, size_t size, uint8_t* buffer) override;
    virtual fk::core::Result<size_t, fk::core::Error> write(
        [[maybe_unused]] uint64_t offset,
        [[maybe_unused]] size_t size,
        [[maybe_unused]] const uint8_t* buffer) override {
        return fk::core::Error::NotImplemented;
    }
    virtual size_t size() const override { return sizeof(SignalfdSiginfo); }
    virtual bool is_signalfd() const override { return true; }
    virtual short poll() const override;

    void update_mask(uint64_t mask);
    bool try_enqueue(int signum);
    uint64_t mask() const { return m_mask; }
    void set_nonblock(bool v) { m_nonblock = v; }

private:
    static constexpr size_t MAX_QUEUE = 64;

    mutable fk::synchronization::Spinlock m_lock;
    [[maybe_unused]] Task* m_task;
    uint64_t m_mask;
    fk::containers::Vector<SignalfdSiginfo> m_queue;
    ipc::Notification m_readable;
    bool m_nonblock{false};
};

} // namespace fkernel
