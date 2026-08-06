#include <LibFK/Container/Sequence/vector.h>

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
    auto existing = m_table.get(ip.value);
    if (existing.has_value()) {
        existing.value().mac = mac;
        existing.value().created_at_ticks = now;
        m_table.insert(ip.value, existing.value());
        return;
    }
    expire_old_entries();
    ArpEntry entry;
    entry.mac = mac;
    entry.created_at_ticks = now;
    m_table.insert(ip.value, entry);
}

void ArpTable::expire_old_entries() {
    uint32_t freq = TickManager::the().get_frequency();
    uint64_t now  = TickManager::the().get_ticks();
    uint64_t ttl_ticks = (freq > 0) ? (ARP_TTL_SECONDS * freq) : UINT64_MAX;

    fk::containers::Vector<uint32_t> to_remove;
    m_table.for_each([&](const uint32_t& key, const ArpEntry& e) {
        if (now - e.created_at_ticks > ttl_ticks)
            TRY_OR_FATAL(to_remove.push_back(key));
    });
    for (size_t i = 0; i < to_remove.size(); ++i)
        m_table.remove(to_remove[i]);
}

fk::memory::optional<MACAddress> ArpTable::lookup(IPv4Address ip) const {
    auto entry = m_table.get(ip.value);
    if (entry.has_value())
        return fk::memory::optional<MACAddress>(entry.value().mac);
    return fk::memory::optional<MACAddress>();
}

void ArpTable::remove(IPv4Address ip) {
    m_table.remove(ip.value);
}

} // namespace net
} // namespace fkernel
