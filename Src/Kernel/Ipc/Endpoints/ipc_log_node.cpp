#include <Kernel/Ipc/Endpoints/ipc_log_node.h>
#include <LibFK/Utilities/memory.h>
#include <LibFK/Memory/Allocators/new.h>

namespace fkernel {

IpcLogNode::IpcLogNode() {
}

fk::RefPtr<IpcLogNode> IpcLogNode::the() {
    static fk::RefPtr<IpcLogNode> instance = fk::make_ref<IpcLogNode>().value();
    return instance;
}

void IpcLogNode::append(const char* str, size_t len) {
    fk::synchronization::ScopedLockIRQ lock(m_lock);
    for (size_t i = 0; i < len; ++i) {
        if (m_buffer.size() >= MAX_LOG_SIZE)
            m_buffer.clear();
        m_buffer.push_back(static_cast<uint8_t>(str[i]));
    }
}

void IpcLogNode::format_log_entry(const char *type, const char *operation, uint32_t task_id, 
                                  const char *details) {
    char log_entry[256];
    int len;
    
    if (details) {
        len = snprintf(log_entry, sizeof(log_entry), "[%s] %s - Task:%u %s\n", 
                      type, operation, task_id, details);
    } else {
        len = snprintf(log_entry, sizeof(log_entry), "[%s] %s - Task:%u\n", 
                      type, operation, task_id);
    }
    
    if (len > 0) {
        append(log_entry, len);
    }
}

void IpcLogNode::log_endpoint_operation(const char *operation, uint32_t sender_id, 
                                        uint32_t receiver_id, uint32_t msg_info) {
    char details[128];
    snprintf(details, sizeof(details), "Sender:%u Receiver:%u MsgInfo:0x%x", 
             sender_id, receiver_id, msg_info);
    format_log_entry("ENDPOINT", operation, sender_id, details);
}

void IpcLogNode::log_notification_operation(const char *operation, uint32_t task_id, uint64_t bits) {
    char details[64];
    snprintf(details, sizeof(details), "Bits:0x%lx", bits);
    format_log_entry("NOTIFICATION", operation, task_id, details);
}

void IpcLogNode::log_signal_delivery(uint32_t task_id, int signal, uint64_t trampoline) {
    char details[128];
    snprintf(details, sizeof(details), "Signal:%d Trampoline:0x%lx", signal, trampoline);
    format_log_entry("SIGNAL", "deliver", task_id, details);
}

fk::core::Result<size_t, fk::core::Error> IpcLogNode::read(uint64_t offset, size_t size, uint8_t* buffer) {
    if (offset >= m_buffer.size()) return 0;
    size_t to_read = (offset + size > m_buffer.size()) ? (m_buffer.size() - offset) : size;
    fk::memory::copy(buffer, m_buffer.begin() + offset, to_read);
    return to_read;
}

fk::core::Result<size_t, fk::core::Error> IpcLogNode::write(uint64_t, size_t size, const uint8_t* buffer) {
    append(reinterpret_cast<const char*>(buffer), size);
    return size;
}

size_t IpcLogNode::size() const {
    return m_buffer.size();
}

} // namespace fkernel