#pragma once

#include <Kernel/Driver/Device/driver_manager.h>
#include <Kernel/Driver/Storage/storage_device.h>
#include <Kernel/Hardware/Pci/pci_device.h>
#include <LibFK/Types/types.h>

namespace fkernel {

/// @brief AHCI (Advanced Host Controller Interface) SATA Controller
/// 
/// Implements AHCI 1.3 specification for SATA storage devices.
/// Provides block device interface through VFS integration.
class AHCIController final : public Driver, public StorageDevice {
public:
    /// @brief Factory method for creating AHCI controller from PCI device
    /// @param device PCI device that represents AHCI controller
    /// @return RefPtr to created controller or nullptr on failure
    static fk::RefPtr<AHCIController> create(const PciDevice& device);
    
    virtual ~AHCIController() override;

    // Driver interface
    virtual const char* name() const override { return "AHCIController"; }
    virtual void probe() override;

    // BlockDevice/StorageDevice interface
    virtual fk::core::Result<size_t, fk::core::Error>
    read_sectors(uint64_t start_sector, size_t count, uint8_t *buffer) override;
    virtual fk::core::Result<size_t, fk::core::Error>
    write_sectors(uint64_t start_sector, size_t count, const uint8_t *buffer) override;

    virtual SectorSize sector_size() const override { return SectorSize(512); }
    virtual SectorCount sector_count() const override;

    // Node interface (VFS integration)
    virtual fk::core::Result<size_t, fk::core::Error> 
    read(uint64_t offset, size_t size, uint8_t* buffer) override;
    virtual fk::core::Result<size_t, fk::core::Error> 
    write(uint64_t offset, size_t size, const uint8_t* buffer) override;
    virtual size_t size() const override;
    virtual bool is_block_device() const override { return true; }

public:
    AHCIController(const PciDevice& device);
    
    // AHCI Structures
    struct HBA_PORT {
        uint32_t clb;       // 0x00, command list base address, 1K-byte aligned
        uint32_t clbu;      // 0x04, command list base address upper 32 bits
        uint32_t fb;        // 0x08, FIS base address, 256-byte aligned
        uint32_t fbu;       // 0x0C, FIS base address upper 32 bits
        uint32_t is;        // 0x10, interrupt status
        uint32_t ie;        // 0x14, interrupt enable
        uint32_t cmd;       // 0x18, command and status
        uint32_t rsv0;      // 0x1C, reserved
        uint32_t tfd;       // 0x20, task file data
        uint32_t sig;       // 0x24, signature
        uint32_t ssts;      // 0x28, SATA status (SCR0:SStatus)
        uint32_t sctl;      // 0x2C, SATA control (SCR2:SControl)
        uint32_t serr;      // 0x30, SATA error (SCR1:SError)
        uint32_t sact;      // 0x34, SATA active (SCR3:SActive)
        uint32_t ci;        // 0x38, command issue
        uint32_t sntf;      // 0x3C, SATA notification (SCR4:SNotification)
        uint32_t fbs;       // 0x40, FIS-based switch control
        uint32_t rsv1[11];  // 0x44 ~ 0x6F, reserved
        uint32_t vendor[4]; // 0x70 ~ 0x7F, vendor specific
    } __attribute__((packed));

    struct HBA_PRDT_ENTRY {
        uint32_t dba;       // Data base address
        uint32_t dbau;      // Data base address upper 32 bits
        uint32_t reserved0; // Reserved
        uint32_t dbc:22;    // Byte count, 4M max
        uint32_t reserved1:9;
        uint32_t i:1;       // Interrupt on completion
    } __attribute__((packed));

    struct HBA_CMD_HEADER {
        uint8_t  cfl:5;     // Command FIS length in DWORDS, 2 ~ 16
        uint8_t  a:1;       // ATAPI
        uint8_t  w:1;       // Write, 1: H2D, 0: D2H
        uint8_t  p:1;       // Prefetchable
        uint8_t  r:1;       // Reset
        uint8_t  b:1;       // BIST
        uint8_t  c:1;       // Clear busy upon R_OK
        uint8_t  reserved0:1;
        uint8_t  pmp:4;     // Port multiplier port
        uint16_t prdtl;     // Physical region descriptor table length in entries
        volatile uint32_t prdbc; // Physical region descriptor byte count transferred
        uint32_t ctba;      // Command table descriptor base address
        uint32_t ctbau;     // Command table descriptor base address upper 32 bits
        uint32_t reserved1[4];
    } __attribute__((packed));

    struct FIS_REG_H2D {
        uint8_t  fis_type;  // FIS_TYPE_REG_H2D (0x27)
        uint8_t  pmport:4;  // Port multiplier
        uint8_t  reserved0:3;
        uint8_t  c:1;       // 1: Command, 0: Control
        uint8_t  command;   // Command register
        uint8_t  featurel;  // Feature register low
        uint8_t  lba0;      // LBA low register, 7:0
        uint8_t  lba1;      // LBA mid register, 15:8
        uint8_t  lba2;      // LBA high register, 23:16
        uint8_t  device;    // Device register
        uint8_t  lba3;      // LBA register, 31:24
        uint8_t  lba4;      // LBA register, 39:32
        uint8_t  lba5;      // LBA register, 47:40
        uint8_t  featureh;  // Feature register high
        uint8_t  countl;    // Count register, 7:0
        uint8_t  counth;    // Count register, 15:8
        uint8_t  icc;       // Isochronous command completion
        uint8_t  control;   // Control register
        uint8_t  reserved1[4];
    } __attribute__((packed));

    struct HBA_CMD_TABLE {
        uint8_t  cfis[64];  // Command FIS
        uint8_t  acmd[16];  // ATAPI command, 12 or 16 bytes
        uint8_t  reserved[48];
        HBA_PRDT_ENTRY prdt_entry[1]; // Flexible array member
    } __attribute__((packed));

    // AHCI initialization and management
    fk::core::Result<void, fk::core::Error> initialize_controller();
    fk::core::Result<void, fk::core::Error> reset_controller();
    void configure_interrupts();
    void scan_ports();
    
    fk::core::Result<void, fk::core::Error> read_port(uint32_t port_idx, uint64_t start_sector, uint32_t count, void* buffer);
    fk::core::Result<void, fk::core::Error> write_port(uint32_t port_idx, uint64_t start_sector, uint32_t count, const void* buffer);

    // Port management
    struct Port {
        uint32_t index;
        bool is_implemented;
        bool has_device;
        uint32_t sig;  // Port signature (SATA, ATAPI, etc.)
        uint64_t sectors;
        volatile HBA_PORT* regs;
    };

private:
    // AHCI registers and memory management
    volatile uint8_t* m_hba_base{nullptr};
    uint32_t m_capabilities{0};
    uint32_t m_version{0};
    fk::containers::Vector<Port> m_ports;
    
    PciDevice m_pci_device;
    bool m_initialized{false};

    int find_cmd_slot(uint32_t port_idx);
    
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
    
    // PCI BAR for AHCI HBA
    static constexpr uint8_t AHCI_PCI_BAR = 0x05;

    // ATA Commands
    static constexpr uint8_t ATA_CMD_READ_DMA_EX = 0x25;
    static constexpr uint8_t ATA_CMD_WRITE_DMA_EX = 0x35;
    static constexpr uint8_t FIS_TYPE_REG_H2D = 0x27;
};

} // namespace fkernel