#pragma once

#include <Kernel/Driver/Device/CharacterDevice/character_device.h>
#include <Kernel/Driver/Network/mac_address.h>
#include <LibFK/Core/result.h>
#include <LibFK/Core/error.h>

namespace fkernel {

class NetworkDevice : public CharacterDevice {
public:
    virtual ~NetworkDevice() override = default;

    virtual fk::core::Result<void, fk::core::Error> send_packet(const uint8_t* data, size_t size) = 0;
    virtual fk::core::Result<size_t, fk::core::Error> receive_packet(uint8_t* buffer, size_t max_size) = 0;
    virtual MACAddress mac_address() const = 0;

    virtual bool is_network_device() const { return true; }

protected:
    NetworkDevice() = default;
};

} // namespace fkernel
