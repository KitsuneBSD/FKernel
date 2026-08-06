#pragma once

#include <Kernel/Fs/Vfs/Core/node.h>
#include <LibFK/Core/result.h>
#include <LibFK/Utilities/memory.h>

// Minimal concrete Node for host-side unit tests.
// Stores up to 512 bytes; acts as a regular file (not a directory).
class MockFileNode : public Node {
    uint8_t  m_data[512]{};
    size_t   m_size{0};

public:
    explicit MockFileNode(size_t initial_size = 64) : m_size(initial_size) {
        for (size_t i = 0; i < m_size && i < sizeof(m_data); ++i)
            m_data[i] = static_cast<uint8_t>(i & 0xFF);
    }

    fk::core::Result<size_t, fk::core::Error>
    read(uint64_t offset, size_t size, uint8_t* buf) override {
        if (offset >= m_size) return size_t(0);
        size_t avail   = m_size - static_cast<size_t>(offset);
        size_t to_read = avail < size ? avail : size;
        fk::memory::copy(buf, m_data + offset, to_read);
        return to_read;
    }

    fk::core::Result<size_t, fk::core::Error>
    write(uint64_t offset, size_t size, const uint8_t* buf) override {
        if (offset >= sizeof(m_data)) return fk::core::Error::InvalidParameter;
        size_t avail    = sizeof(m_data) - static_cast<size_t>(offset);
        size_t to_write = avail < size ? avail : size;
        fk::memory::copy(m_data + offset, buf, to_write);
        size_t end = static_cast<size_t>(offset) + to_write;
        if (end > m_size) m_size = end;
        return to_write;
    }

    size_t size() const override { return m_size; }
};
