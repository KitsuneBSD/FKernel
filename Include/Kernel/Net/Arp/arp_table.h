#pragma once

#include <Kernel/Driver/Network/mac_address.h>
#include <Kernel/Net/Arp/arp_entry.h>
#include <Kernel/Net/Ip/ip_address.h>
#include <LibFK/Container/Associative/hash_map.h>
#include <LibFK/Memory/optional.h>
#include <LibFK/Types/types.h>

namespace fkernel {
namespace net {

class ArpTable {
  fk::containers::HashMap<uint32_t, ArpEntry> m_table;

public:
  static ArpTable& the();

  void update(IPv4Address ip, const MACAddress& mac);
  fk::memory::optional<MACAddress> lookup(IPv4Address ip) const;
  void remove(IPv4Address ip);
  void expire_old_entries();
};

} // namespace net
} // namespace fkernel
