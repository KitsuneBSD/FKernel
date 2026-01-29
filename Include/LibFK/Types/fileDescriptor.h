#pragma once

#include <LibFK/Types/types.h>
#include <LibFK/Core/Assertions.h>

namespace fk {

class FileDescriptor {
public:
    constexpr FileDescriptor() : m_fd(-1) {}
    constexpr explicit FileDescriptor(int fd) : m_fd(fd) {
        ASSERT(fd >= -1 && fd < 1024); // Kernel FD limit
    }

    int value() const { return m_fd; }
    bool is_valid() const { return m_fd >= 0; }

    bool operator==(const FileDescriptor& other) const { return m_fd == other.m_fd; }
    bool operator!=(const FileDescriptor& other) const { return m_fd != other.m_fd; }

private:
    int m_fd;
};

} // namespace fk
