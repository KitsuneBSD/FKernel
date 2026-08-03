#pragma once

#include <Kernel/Driver/Usb/usb_transfer_type.h>
#include <Kernel/Driver/Usb/usb_transfer_direction.h>
#include <LibFK/Types/types.h>

class USBTransfer {
public:
    USBTransfer() = default;
    virtual ~USBTransfer() = default;

    USBTransferType type() const { return m_type; }
    void set_type(USBTransferType t) { m_type = t; }

    USBTransferDirection direction() const { return m_direction; }
    void set_direction(USBTransferDirection d) { m_direction = d; }

    uint8_t endpoint_address() const { return m_endpoint_address; }
    void set_endpoint_address(uint8_t addr) { m_endpoint_address = addr; }

private:
    USBTransferType m_type{USBTransferType::Control};
    USBTransferDirection m_direction{USBTransferDirection::In};
    uint8_t m_endpoint_address{0};
};
