#pragma once

#include <LibFK/Types/types.h>

namespace fk {

class FileDescriptor {
public:
    static constexpr uint32_t INVALID = 0xFFFFFFFF;

    constexpr FileDescriptor() : m_fd(INVALID) {}
    
    constexpr explicit FileDescriptor(uint32_t fd)
        : m_fd(fd < 1024 || fd == INVALID ? fd : INVALID) {}

    uint32_t value() const { 
        return m_fd; 
    }

    bool is_valid() const { return m_fd != INVALID; }

    bool operator==(const FileDescriptor& other) const { return m_fd == other.m_fd; }
    bool operator!=(const FileDescriptor& other) const { return m_fd != other.m_fd; }

private:
    uint32_t m_fd;
};

} // namespace fk
