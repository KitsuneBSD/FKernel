#pragma once

#include <LibFK/Core/Result.h>
#include <LibFK/Core/Error.h>
#include <LibC/stdint.h>
#include <Kernel/Driver/Usb/usb_transfer.h>
#include <Kernel/Driver/Usb/usb_device.h>

class USBHostController {
public:
    virtual ~USBHostController() = default;

    virtual fk::core::Result<void, fk::core::Error> initialize() = 0;
    
    // Enumerates a device on a specific port.
    // Returns a pointer to the device if successful.
    virtual fk::core::Result<USBDevice*, fk::core::Error> enumerate_device(uint8_t port) = 0;
    
    virtual fk::core::Result<void, fk::core::Error> submit_transfer(USBTransfer* transfer) = 0;
};
