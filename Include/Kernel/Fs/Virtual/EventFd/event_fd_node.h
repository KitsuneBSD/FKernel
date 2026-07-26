#pragma once

#include <Kernel/Fs/Vfs/node.h>
#include <Kernel/Ipc/notification.h>
#include <LibFK/Core/result.h>
#include <LibFK/Types/types.h>

namespace fkernel {

class EventFdNode final : public Node {
public:
    static constexpr int EFD_SEMAPHORE = 1;
    static constexpr int EFD_NONBLOCK  = 2048;

    static fk::core::Result<fk::RefPtr<EventFdNode>, fk::core::Error> create(
        uint64_t initial_value, int flags);

    explicit EventFdNode(uint64_t initial_value, bool semaphore, bool nonblock);
    virtual ~EventFdNode() override = default;

    virtual fk::core::Result<size_t, fk::core::Error> read(
        uint64_t offset, size_t size, uint8_t* buffer) override;
    virtual fk::core::Result<size_t, fk::core::Error> write(
        uint64_t offset, size_t size, const uint8_t* buffer) override;
    virtual size_t size() const override { return sizeof(uint64_t); }
    virtual bool is_eventfd() const override { return true; }
    virtual short poll() const override;

private:
    static constexpr uint64_t MAX_COUNTER = 0xFFFFFFFFFFFFFFFEULL;

    mutable fk::synchronization::Spinlock m_lock;
    uint64_t m_counter;
    bool m_is_semaphore;
    bool m_nonblock;
    ipc::Notification m_readable;
};

}
