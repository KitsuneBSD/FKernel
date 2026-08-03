#pragma once

#include <Kernel/Driver/Device/driver_manager.h>
#include <Kernel/Driver/Storage/Interfaces/storage_device.h>
#include <Kernel/Hardware/Buses/Pci/pci_device.h>
#include <Kernel/Memory/Dma/dma_buffer.h>
#include <LibFK/Types/types.h>

namespace fkernel {

/// @brief NVMe (NVM Express) Controller
/// 
/// Implements NVMe 1.4 specification for PCIe-based SSDs.
/// Provides block device interface through VFS integration.
class NVMeController final : public Driver, public StorageDevice {
public:
    /// @brief Factory method for creating NVMe controller from PCI device
    /// @param device PCI device that represents NVMe controller
    /// @return RefPtr to created controller or nullptr on failure
    static fk::RefPtr<NVMeController> create(const PciDevice& device);
    
    virtual ~NVMeController() override;

    // Driver interface
    virtual const char* name() const override { return "NVMeController"; }
    virtual void probe() override;

    // BlockDevice/StorageDevice interface
    virtual fk::core::Result<size_t, fk::core::Error>
    read_sectors(uint64_t start_sector, size_t count, uint8_t *buffer) override;
    virtual fk::core::Result<size_t, fk::core::Error>
    write_sectors(uint64_t start_sector, size_t count, const uint8_t *buffer) override;

    virtual SectorSize sector_size() const override {
        return SectorSize(m_namespaces.is_empty() ? 512u : m_namespaces[0].block_size);
    }
    virtual SectorCount sector_count() const override;

    // Node interface (VFS integration)
    virtual fk::core::Result<size_t, fk::core::Error> 
    read(uint64_t offset, size_t size, uint8_t* buffer) override;
    virtual fk::core::Result<size_t, fk::core::Error> 
    write(uint64_t offset, size_t size, const uint8_t* buffer) override;
    virtual size_t size() const override;
    virtual bool is_block_device() const override { return true; }

public:
    NVMeController(const PciDevice& device);
    
    // NVMe initialization and management
    fk::core::Result<void, fk::core::Error> initialize_controller();
    fk::core::Result<void, fk::core::Error> reset_controller();
    fk::core::Result<void, fk::core::Error> identify_controller();
    fk::core::Result<void, fk::core::Error> configure_admin_queue();
    fk::core::Result<void, fk::core::Error> create_io_queues();
    void scan_namespaces();
    
    // NVMe namespace structure
    struct Namespace {
        uint32_t nsid;
        uint64_t size_blocks;
        uint32_t block_size;
        bool active;
    };
    
    // NVMe queue structure
    struct QueuePair {
        volatile uint32_t* sq_tail_db{nullptr};  // Submission queue tail doorbell
        volatile uint32_t* cq_head_db{nullptr};  // Completion queue head doorbell
        DmaBuffer sq_buffer;                      // Submission queue DMA buffer
        DmaBuffer cq_buffer;                      // Completion queue DMA buffer
        void* sq_memory{nullptr};                // CPU virtual pointer to SQ
        void* cq_memory{nullptr};                // CPU virtual pointer to CQ
        uint16_t sq_size{0};
        uint16_t cq_size{0};
        uint16_t sq_tail{0};
        uint16_t cq_head{0};
        uint16_t cq_phase{1};
    };
    
    QueuePair m_admin_queue;
    QueuePair m_io_queue;
    
    volatile uint8_t* m_controller_regs{nullptr};
    uint64_t m_controller_capabilities{0};
    uint32_t m_version{0};

    PciDevice m_pci_device;
    bool m_initialized{false};
    
    // NVMe Controller register offsets
    static constexpr uint64_t NVME_CAP = 0x0000;  // Controller Capabilities
    static constexpr uint64_t NVME_VS = 0x0008;   // Version
    static constexpr uint64_t NVME_INTMS = 0x000C; // Interrupt Mask Set
    static constexpr uint64_t NVME_INTMC = 0x0010; // Interrupt Mask Clear
    static constexpr uint64_t NVME_CC = 0x0014;    // Controller Configuration
    static constexpr uint64_t NVME_CSTS = 0x001C;  // Controller Status
    static constexpr uint64_t NVME_NSSR = 0x0020;  // NVM Subsystem Reset
    static constexpr uint64_t NVME_AQA = 0x0024;  // Admin Queue Attributes
    static constexpr uint64_t NVME_ASQ = 0x0028;  // Admin Submission Queue Base Address
    static constexpr uint64_t NVME_ACQ = 0x0030;  // Admin Completion Queue Base Address
    
    // Doorbell offsets (stride calculated from CAP)
    uint32_t m_doorbell_stride{0};
    
    // NVMe Command structures (simplified)
    struct Command {
        uint32_t cdw0;
        uint32_t nsid;
        uint32_t rsvd2[2];
        uint64_t metadata;
        uint64_t prp1;
        uint64_t prp2;
        uint32_t cdw10;
        uint32_t cdw11;
        uint32_t cdw12;
        uint32_t cdw13;
        uint32_t cdw14;
        uint32_t cdw15;
    } __attribute__((packed));
    
    struct Completion {
        uint32_t cdw0;
        uint32_t rsvd;
        uint16_t sq_head;
        uint16_t sq_id;
        uint16_t cid;
        uint16_t status;
    } __attribute__((packed));
    
    fk::core::Result<void, fk::core::Error> submit_command(QueuePair& queue, Command& cmd);

    // PCI BAR for NVMe controller
    static constexpr uint8_t NVME_PCI_BAR = 0x00;
    
    // NVMe Command Opcodes
    static constexpr uint8_t NVME_CMD_CREATE_IO_SQ = 0x01;
    static constexpr uint8_t NVME_CMD_CREATE_IO_CQ = 0x05;
    static constexpr uint8_t NVME_CMD_IDENTIFY = 0x06;
    static constexpr uint8_t NVME_CMD_WRITE = 0x01;
    static constexpr uint8_t NVME_CMD_READ = 0x02;

    fk::containers::Vector<Namespace> m_namespaces;
};

} // namespace fkernel