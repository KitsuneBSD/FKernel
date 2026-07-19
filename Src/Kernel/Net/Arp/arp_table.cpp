#include <Kernel/Net/Arp/arp_table.h>

namespace fkernel {
namespace net {

ArpTable& ArpTable::the() {
    static ArpTable instance;
    return instance;
}

void ArpTable::update(IPv4Address ip, const MACAddress& mac) {
    for (auto& entry : m_entries) {
        if (entry.ip == ip) { entry.mac = mac; return; }
    }
    m_entries.push_back({ip, mac});
}

fk::memory::optional<MACAddress> ArpTable::lookup(IPv4Address ip) const {
    for (size_t i = 0; i < m_entries.size(); ++i) {
        if (m_entries[i].ip == ip)
            return fk::memory::optional<MACAddress>(m_entries[i].mac);
    }
    return fk::memory::optional<MACAddress>();
}

void ArpTable::remove(IPv4Address ip) {
    for (size_t i = 0; i < m_entries.size(); ++i) {
        if (m_entries[i].ip == ip) { m_entries.remove_at(i); return; }
    }
}

} // namespace net
} // namespace fkernel
