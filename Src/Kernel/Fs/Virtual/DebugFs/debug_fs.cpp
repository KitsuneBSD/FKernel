#include <Kernel/Fs/Virtual/DebugFs/debug_fs.h>
#include <LibFK/Utilities/memory.h>
#include <LibFK/Memory/new.h>

namespace fkernel {

DebugLogNode::DebugLogNode() {
}

fk::RefPtr<DebugLogNode> DebugLogNode::the() {
    static fk::RefPtr<DebugLogNode> instance = fk::make_ref<DebugLogNode>().value();
    return instance;
}

void DebugLogNode::append(const char* str, size_t len) {
    fk::synchronization::ScopedLockIRQ lock(m_lock);
    for (size_t i = 0; i < len; ++i) {
        if (m_buffer.size() >= MAX_LOG_SIZE) {
            m_buffer.clear();
        }
        m_buffer.push_back(static_cast<uint8_t>(str[i]));
    }
}

fk::core::Result<size_t, fk::core::Error> DebugLogNode::read(uint64_t offset, size_t size, uint8_t* buffer) {
    fk::synchronization::ScopedLockIRQ lock(m_lock);
    if (offset >= m_buffer.size()) return 0;
    size_t to_read = (offset + size > m_buffer.size()) ? (m_buffer.size() - offset) : size;
    fk::memory::copy(buffer, m_buffer.begin() + offset, to_read);
    return to_read;
}

fk::core::Result<size_t, fk::core::Error> DebugLogNode::write(uint64_t, size_t size, const uint8_t* buffer) {
    append(reinterpret_cast<const char*>(buffer), size);
    return size;
}

size_t DebugLogNode::size() const {
    return m_buffer.size();
}

SyscallLogNode::SyscallLogNode() {}

fk::RefPtr<SyscallLogNode> SyscallLogNode::the() {
    static fk::RefPtr<SyscallLogNode> instance = fk::make_ref<SyscallLogNode>().value();
    return instance;
}

void SyscallLogNode::append(const char* str, size_t len) {
    fk::synchronization::ScopedLockIRQ lock(m_lock);
    for (size_t i = 0; i < len; ++i) {
        if (m_buffer.size() >= MAX_LOG_SIZE) m_buffer.clear();
        m_buffer.push_back(static_cast<uint8_t>(str[i]));
    }
}

fk::core::Result<size_t, fk::core::Error> SyscallLogNode::read(uint64_t offset, size_t size, uint8_t* buffer) {
    fk::synchronization::ScopedLockIRQ lock(m_lock);
    if (offset >= m_buffer.size()) return 0;
    size_t to_read = (offset + size > m_buffer.size()) ? (m_buffer.size() - offset) : size;
    fk::memory::copy(buffer, m_buffer.begin() + offset, to_read);
    return to_read;
}

fk::core::Result<size_t, fk::core::Error> SyscallLogNode::write(uint64_t, size_t size, const uint8_t* buffer) {
    append(reinterpret_cast<const char*>(buffer), size);
    return size;
}

size_t SyscallLogNode::size() const {
    return m_buffer.size();
}

DebugFsNode::DebugFsNode() {}

fk::core::Result<fk::RefPtr<Node>, fk::core::Error> DebugFsNode::lookup(const char* name) {
    if (fk::memory::compare(name, "klog") == 0) {
        return fk::RefPtr<Node>(DebugLogNode::the());
    }
    if (fk::memory::compare(name, "syscalls") == 0) {
        return fk::RefPtr<Node>(SyscallLogNode::the());
    }
    if (fk::memory::compare(name, "ipc") == 0) {
        return fk::RefPtr<Node>(IpcLogNode::the());
    }
    return fk::core::Error::NotFound;
}

fk::core::Result<void, fk::core::Error> DebugFsNode::list_dir(fk::containers::Vector<DirectoryEntry>& entries) {
    DirectoryEntry klog_de;
    fk::memory::copy_string(klog_de.name, "klog");
    klog_de.type = 0;
    entries.push_back(klog_de);

    DirectoryEntry syscalls_de;
    fk::memory::copy_string(syscalls_de.name, "syscalls");
    syscalls_de.type = 0;
    entries.push_back(syscalls_de);

    DirectoryEntry ipc_de;
    fk::memory::copy_string(ipc_de.name, "ipc");
    ipc_de.type = 0;
    entries.push_back(ipc_de);

    return {};
}

} // namespace fkernel