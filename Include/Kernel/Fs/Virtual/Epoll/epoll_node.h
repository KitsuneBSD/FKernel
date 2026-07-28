#pragma once

#include <Kernel/Fs/Vfs/kqueue.h>
#include <Kernel/Fs/Vfs/node.h>
#include <LibFK/Container/vector.h>
#include <LibFK/Memory/ref_ptr.h>

namespace fkernel {

struct EpollEntry {
    int      fd;
    uint32_t events;
    uint64_t data_u64;
};

class EpollNode : public Node {
    fk::containers::Vector<EpollEntry> m_entries;
    fk::RefPtr<KQueueNode> m_kq;

public:
    fk::core::Result<size_t, fk::core::Error> read(uint64_t, size_t, uint8_t*) override {
        return fk::core::Error::NotImplemented;
    }
    fk::core::Result<size_t, fk::core::Error> write(uint64_t, size_t, const uint8_t*) override {
        return fk::core::Error::NotImplemented;
    }
    size_t size() const override { return 0; }
    short poll() const override { return 0; }

    bool ctl_add(int fd, uint32_t events, uint64_t data);
    bool ctl_del(int fd);
    bool ctl_mod(int fd, uint32_t events, uint64_t data);

    const fk::containers::Vector<EpollEntry>& entries() const { return m_entries; }
    fk::RefPtr<KQueueNode> kqueue_node() { return m_kq; }
};

} // namespace fkernel
