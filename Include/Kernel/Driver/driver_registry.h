#pragma once

#include <LibFK/Types/types.h>

namespace fkernel {

class DriverRegistry {
public:
    static void initialize();
    
    /// @brief Register all storage drivers (ATA, AHCI, NVMe)
    static void register_storage_drivers();
    
    /// @brief Register all network drivers (future)
    static void register_network_drivers();
    
    /// @brief Register all display drivers (future)
    static void register_display_drivers();

private:
    DriverRegistry() = delete;
    
    /// @brief Template helper for registering PCI drivers
    template<typename ControllerClass>
    static void register_pci_driver(uint8_t class_code, uint8_t subclass);
};

} // namespace fkernel