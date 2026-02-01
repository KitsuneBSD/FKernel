#include <Kernel/Driver/driver_registry.h>
#include <Kernel/Hardware/Pci/pci.h>
#include <Kernel/Driver/Storage/Ata/ata_controller.h>
#include <Kernel/Driver/Storage/Ahci/ahci_controller.h>
#include <Kernel/Driver/Storage/Nvme/nvme_controller.h>
#include <Kernel/Driver/Network/E1000/e1000.h>
#include <LibFK/Traits/type_traits.h>
#include <LibFK/Algorithms/log.h>

namespace fkernel {

void DriverRegistry::initialize() {
    fk::algorithms::klog("DRIVER REGISTRY", "Initializing auto-registration system...");
    
    register_storage_drivers();
    register_network_drivers();
    register_display_drivers();
    
    fk::algorithms::klog("DRIVER REGISTRY", "Auto-registration complete.");
}

void DriverRegistry::register_storage_drivers() {
    fk::algorithms::klog("DRIVER REGISTRY", "Registering storage drivers...");
    
    // PCI Class 0x01: Mass Storage Controllers
    register_pci_driver<ATAController>(0x01, 0x01);  // IDE Controller
    register_pci_driver<AHCIController>(0x01, 0x06);  // SATA Controller  
    register_pci_driver<NVMeController>(0x01, 0x08);  // NVMe Controller
    
    fk::algorithms::klog("DRIVER REGISTRY", "Storage drivers registered.");
}

void DriverRegistry::register_network_drivers() {
    fk::algorithms::klog("DRIVER REGISTRY", "Registering network drivers...");
    
    // PCI Class 0x02: Network Controllers
    register_pci_driver<E1000Controller>(0x02, 0x00); // Ethernet Controller
    
    fk::algorithms::klog("DRIVER REGISTRY", "Network drivers registered.");
}

void DriverRegistry::register_display_drivers() {
    fk::algorithms::klog("DRIVER REGISTRY", "Display drivers not implemented yet.");
    // Future: register_pci_driver<VGADriver>(0x03, 0x00);
}

template<typename ControllerClass>
void DriverRegistry::register_pci_driver(uint8_t class_code, uint8_t subclass) {
    PciManager::the().register_driver(class_code, subclass, [](const PciDevice& device) -> fk::RefPtr<Node> {
        if constexpr (fk::traits::is_same_v<ControllerClass, ATAController>) {
            // ATA uses singleton pattern
            ATAController::the().detect_on_pci(device);
            return nullptr; 
        } else {
            // Other controllers use factory pattern
            auto controller = ControllerClass::create(device);
            if (controller) {
                DriverManager::the().register_device(controller);
            }
            return controller;
        }
    });
}

// Explicit template instantiations
template void DriverRegistry::register_pci_driver<ATAController>(uint8_t, uint8_t);
template void DriverRegistry::register_pci_driver<AHCIController>(uint8_t, uint8_t);
template void DriverRegistry::register_pci_driver<NVMeController>(uint8_t, uint8_t);
template void DriverRegistry::register_pci_driver<E1000Controller>(uint8_t, uint8_t);

} // namespace fkernel