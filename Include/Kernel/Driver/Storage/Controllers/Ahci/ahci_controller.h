#pragma once

#include <Kernel/Driver/Device/driver_manager.h>
#include <Kernel/Driver/Storage/Controllers/Ahci/ahci_port.h>
#include <Kernel/Driver/Storage/Controllers/Ahci/fis_reg_h2d.h>
#include <Kernel/Driver/Storage/Controllers/Ahci/hba_cmd_header.h>
#include <Kernel/Driver/Storage/Controllers/Ahci/hba_cmd_table.h>
#include <Kernel/Driver/Storage/Controllers/Ahci/hba_port.h>
#include <Kernel/Driver/Storage/Controllers/Ahci/hba_prdt_entry.h>
#include <Kernel/Driver/Storage/Interfaces/storage_device.h>
#include <Kernel/Hardware/Buses/Pci/pci_device.h>
#include <Kernel/Driver/Async/dma_buffer.h>
#include <LibFK/Types/types.h>

namespace fkernel {

class AHCIController : public Driver, public StorageDevice {
public:
    static fk::RefPtr<AHCIController> create(const PciDevice& device);

    virtual ~AHCIController() override;

    virtual const char* name() const override { return "AHCIController"; }
    virtual void probe() override;

    virtual fk::core::Result<size_t, fk::core::Error>
    read_sectors(uint64_t start_sector, size_t count, uint8_t *buffer) override;
    virtual fk::core::Result<size_t, fk::core::Error>
    write_sectors(uint64_t start_sector, size_t count, const uint8_t *buffer) override;

    virtual SectorSize sector_size() const override { return SectorSize(512); }
    virtual SectorCount sector_count() const override;

    virtual fk::core::Result<size_t, fk::core::Error>
    read(uint64_t offset, size_t size, uint8_t* buffer) override;
    virtual fk::core::Result<size_t, fk::core::Error>
    write(uint64_t offset, size_t size, const uint8_t* buffer) override;
    virtual size_t size() const override;
    virtual bool is_block_device() const override { return true; }

public:
    AHCIController(const PciDevice& device);

    fk::core::Result<void, fk::core::Error> initialize_controller();
    fk::core::Result<void, fk::core::Error> reset_controller();
    void configure_interrupts();
    void scan_ports();

    fk::core::Result<void, fk::core::Error> read_port(uint32_t port_idx, uint64_t start_sector, uint32_t count, void* buffer);
    fk::core::Result<void, fk::core::Error> write_port(uint32_t port_idx, uint64_t start_sector, uint32_t count, const void* buffer);

protected:
    PciDevice m_pci_device;
    volatile uint8_t* m_hba_base{nullptr};
    fk::containers::Vector<AhciPort> m_ports;

private:
    uint32_t m_capabilities{0};
    uint32_t m_version{0};
    bool m_initialized{false};

    int find_cmd_slot(uint32_t port_idx);
    fk::core::Result<uint64_t, fk::core::Error> identify_port(uint32_t port_idx);

    static constexpr uint32_t HBA_CAP    = 0x00;
    static constexpr uint32_t HBA_GHC    = 0x04;
    static constexpr uint32_t HBA_IS     = 0x08;
    static constexpr uint32_t HBA_PI     = 0x0C;
    static constexpr uint32_t HBA_VS     = 0x10;
    static constexpr uint32_t HBA_CCCC   = 0x14;
    static constexpr uint32_t HBA_CCCP   = 0x18;
    static constexpr uint32_t HBA_EM_LOC = 0x1C;
    static constexpr uint32_t HBA_EM_CTL = 0x20;
    static constexpr uint32_t HBA_CAP2   = 0x24;
    static constexpr uint32_t HBA_BOHC   = 0x28;
    static constexpr uint8_t  AHCI_PCI_BAR = 0x05;
    static constexpr uint8_t  ATA_CMD_READ_DMA_EX  = 0x25;
    static constexpr uint8_t  ATA_CMD_WRITE_DMA_EX = 0x35;
    static constexpr uint8_t  ATA_CMD_IDENTIFY     = 0xEC;
    static constexpr uint8_t  FIS_TYPE_REG_H2D     = 0x27;
};

} // namespace fkernel
