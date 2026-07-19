#pragma once

#include <Kernel/Driver/Network/mac_address.h>
#include <Kernel/Net/Ip/ip_address.h>
#include <LibFK/Container/vector.h>
#include <LibFK/Memory/optional.h>

namespace fkernel {
namespace net {

struct ArpEntry {
    IPv4Address ip;
    MACAddress  mac;
};

class ArpTable {
  fk::containers::Vector<ArpEntry> m_entries;

public:
  static ArpTable& the();

  void update(IPv4Address ip, const MACAddress& mac);
  fk::memory::optional<MACAddress> lookup(IPv4Address ip) const;
  void remove(IPv4Address ip);
};

} // namespace net
} // namespace fkernel
