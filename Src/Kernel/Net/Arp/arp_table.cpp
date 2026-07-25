#include <Kernel/Net/Arp/arp_table.h>
#include <Kernel/Arch/x86_64/Interrupt/HardwareInterrupts/tick_manager.h>

namespace fkernel {
namespace net {

static constexpr uint64_t ARP_TTL_SECONDS = 300;

ArpTable& ArpTable::the() {
    static ArpTable instance;
    return instance;
}

void ArpTable::update(IPv4Address ip, const MACAddress& mac) {
    uint64_t now = TickManager::the().get_ticks();
    for (auto& entry : m_entries) {
        if (entry.ip == ip) {
            entry.mac = mac;
            entry.created_at_ticks = now;
            return;
        }
    }
    expire_old_entries();
    m_entries.push_back({ip, mac, now});
}

void ArpTable::expire_old_entries() {
    uint32_t freq = TickManager::the().get_frequency();
    uint64_t now = TickManager::the().get_ticks();
    uint64_t ttl_ticks = (freq > 0) ? (ARP_TTL_SECONDS * freq) : UINT64_MAX;
    for (size_t i = 0; i < m_entries.size(); ) {
        if (now - m_entries[i].created_at_ticks > ttl_ticks) {
            m_entries.remove_at(i);
            continue;
        }
        ++i;
    }
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
