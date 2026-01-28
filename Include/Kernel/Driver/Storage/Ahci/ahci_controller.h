#pragma once

#include <Kernel/Driver/Device/driver_manager.h>
#include <Kernel/Hardware/Pci/pci_device.h>
#include <Kernel/Fs/Vfs/node.h>
#include <LibFK/Types/types.h>

namespace fkernel {

/// @brief AHCI (Advanced Host Controller Interface) SATA Controller
/// 
/// Implements AHCI 1.3 specification for SATA storage devices.
/// Provides block device interface through VFS integration.
class AHCIController final : public Driver, public Node {
public:
    /// @brief Factory method for creating AHCI controller from PCI device
    /// @param device PCI device that represents AHCI controller
    /// @return RefPtr to created controller or nullptr on failure
    static fk::RefPtr<AHCIController> create(const PciDevice& device);
    
    virtual ~AHCIController() override;

    // Driver interface
    virtual const char* name() const override { return "AHCIController"; }
    virtual void probe() override;

    // Node interface (VFS integration)
    virtual fk::core::Result<size_t, fk::core::Error> 
    read(uint64_t offset, size_t size, uint8_t* buffer) override;
    virtual fk::core::Result<size_t, fk::core::Error> 
    write(uint64_t offset, size_t size, const uint8_t* buffer) override;
    virtual size_t size() const override;
    virtual bool is_block_device() const override { return true; }

public:
    AHCIController(const PciDevice& device);
    
    // AHCI initialization and management
    fk::core::Result<void, fk::core::Error> initialize_controller();
    fk::core::Result<void, fk::core::Error> reset_controller();
    void configure_interrupts();
    void scan_ports();
    
    // Port management
    struct Port {
        uint32_t index;
        bool is_implemented;
        bool has_device;
        uint32_t sig;  // Port signature (SATA, ATAPI, etc.)
    };
    
    // AHCI registers and memory management
    volatile uint8_t* m_hba_base{nullptr};
    uint32_t m_capabilities{0};
    uint32_t m_version{0};
    fk::containers::Vector<Port> m_ports;
    
    PciDevice m_pci_device;
    bool m_initialized{false};
    
    // AHCI register offsets
    static constexpr uint32_t HBA_CAP = 0x00;
    static constexpr uint32_t HBA_GHC = 0x04;
    static constexpr uint32_t HBA_IS = 0x08;
    static constexpr uint32_t HBA_PI = 0x0C;
    static constexpr uint32_t HBA_VS = 0x10;
    static constexpr uint32_t HBA_CCCC = 0x14;
    static constexpr uint32_t HBA_CCCP = 0x18;
    static constexpr uint32_t HBA_EM_LOC = 0x1C;
    static constexpr uint32_t HBA_EM_CTL = 0x20;
    static constexpr uint32_t HBA_CAP2 = 0x24;
    static constexpr uint32_t HBA_BOHC = 0x28;
    
    // Port register offsets (base + 0x100 + (port_index * 0x80))
    static constexpr uint32_t PORT_CLB = 0x00;  // Command List Base Address
    static constexpr uint32_t PORT_FB = 0x08;   // FIS Base Address
    static constexpr uint32_t PORT_IS = 0x10;   // Interrupt Status
    static constexpr uint32_t PORT_IE = 0x14;   // Interrupt Enable
    static constexpr uint32_t PORT_CMD = 0x18;  // Command and Status
    static constexpr uint32_t PORT_TFD = 0x20;  // Task File Data
    static constexpr uint32_t PORT_SIG = 0x24;  // Signature
    static constexpr uint32_t PORT_SSTS = 0x28; // Serial ATA Status
    static constexpr uint32_t PORT_SCTL = 0x2C; // Serial ATA Control
    static constexpr uint32_t PORT_SERR = 0x30; // Serial ATA Error
    static constexpr uint32_t PORT_SACT = 0x34; // Serial ATA Active
    static constexpr uint32_t PORT_CI = 0x38;   // Command Issue
    static constexpr uint32_t PORT_SNTF = 0x3C; // SATA Notification
    
    // PCI BAR for AHCI HBA
    static constexpr uint8_t AHCI_PCI_BAR = 0x05;
};

} // namespace fkernel