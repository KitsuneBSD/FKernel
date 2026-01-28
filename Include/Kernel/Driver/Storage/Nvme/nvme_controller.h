#pragma once

#include <Kernel/Driver/Device/driver_manager.h>
#include <Kernel/Hardware/Pci/pci_device.h>
#include <Kernel/Fs/Vfs/node.h>
#include <LibFK/Types/types.h>

namespace fkernel {

/// @brief NVMe (NVM Express) Controller
/// 
/// Implements NVMe 1.4 specification for PCIe-based SSDs.
/// Provides block device interface through VFS integration.
class NVMeController final : public Driver, public Node {
public:
    /// @brief Factory method for creating NVMe controller from PCI device
    /// @param device PCI device that represents NVMe controller
    /// @return RefPtr to created controller or nullptr on failure
    static fk::RefPtr<NVMeController> create(const PciDevice& device);
    
    virtual ~NVMeController() override;

    // Driver interface
    virtual const char* name() const override { return "NVMeController"; }
    virtual void probe() override;

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
    void configure_admin_queue();
    void scan_namespaces();
    
    // NVMe namespace structure
    struct Namespace {
        uint32_t nsid;
        uint64_t size_blocks;
        uint32_t block_size;
        bool active;
    };
    
    // NVMe registers and memory management
    volatile uint8_t* m_controller_regs{nullptr};
    uint64_t m_controller_capabilities{0};
    uint32_t m_version{0};
    fk::containers::Vector<Namespace> m_namespaces;
    
    // Admin queue
    struct QueuePair {
        volatile uint32_t* sq_tail_db{nullptr};  // Submission queue tail doorbell
        volatile uint32_t* cq_head_db{nullptr};  // Completion queue head doorbell
        void* sq_memory{nullptr};                // Submission queue memory
        void* cq_memory{nullptr};                // Completion queue memory
        uint16_t sq_size{0};
        uint16_t cq_size{0};
        uint16_t sq_phase{1};
        uint16_t cq_phase{1};
    };
    
    QueuePair m_admin_queue;
    
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
    
    // NVMe Command structures
    struct Command {
        uint32_t cdw0;
        uint32_t cdw1;
        uint32_t cdw2;
        uint32_t cdw3;
        uint32_t cdw4;
        uint32_t cdw5;
        uint64_t prp1;
        uint64_t prp2;
    };
    
    struct Completion {
        uint32_t cdw0;
        uint32_t rsvd;
        uint16_t sq_head;
        uint16_t sq_id;
        uint16_t cid;
        uint16_t status;
    };
    
    // PCI BAR for NVMe controller
    static constexpr uint8_t NVME_PCI_BAR = 0x00;
    
    // NVMe Command Opcodes
    static constexpr uint8_t NVME_CMD_DELETE_IO_SQ = 0x00;
    static constexpr uint8_t NVME_CMD_CREATE_IO_SQ = 0x01;
    static constexpr uint8_t NVME_CMD_GET_LOG_PAGE = 0x02;
    static constexpr uint8_t NVME_CMD_DELETE_IO_CQ = 0x04;
    static constexpr uint8_t NVME_CMD_CREATE_IO_CQ = 0x05;
    static constexpr uint8_t NVME_CMD_IDENTIFY = 0x06;
    static constexpr uint8_t NVME_CMD_ABORT = 0x08;
    static constexpr uint8_t NVME_CMD_SET_FEATURES = 0x09;
    static constexpr uint8_t NVME_CMD_GET_FEATURES = 0x0A;
    static constexpr uint8_t NVME_CMD_ASYNC_EVENT = 0x0C;
    static constexpr uint8_t NVME_CMD_NAMESPACE_MGMT = 0x0D;
    static constexpr uint8_t NVME_CMD_FIRMWARE_ACTIVATE = 0x10;
    static constexpr uint8_t NVME_CMD_FIRMWARE_DOWNLOAD = 0x11;
    static constexpr uint8_t NVME_CMD_FORMAT_NVM = 0x80;
    static constexpr uint8_t NVME_CMD_SECURITY_SEND = 0x81;
    static constexpr uint8_t NVME_CMD_SECURITY_RECV = 0x82;
    static constexpr uint8_t NVME_CMD_FLUSH = 0x00;
    static constexpr uint8_t NVME_CMD_WRITE = 0x01;
    static constexpr uint8_t NVME_CMD_READ = 0x02;
    static constexpr uint8_t NVME_CMD_WRITE_UNCORRECTABLE = 0x04;
    static constexpr uint8_t NVME_CMD_COMPARE = 0x05;
    static constexpr uint8_t NVME_CMD_WRITE_ZEROS = 0x08;
    static constexpr uint8_t NVME_CMD_DSM = 0x09;
    static constexpr uint8_t NVME_CMD_VERIFY = 0x0C;
    static constexpr uint8_t NVME_CMD_RESERVATION_REGISTER = 0x0D;
    static constexpr uint8_t NVME_CMD_RESERVATION_REPORT = 0x0E;
    static constexpr uint8_t NVME_CMD_RESERVATION_ACQUIRE = 0x11;
    static constexpr uint8_t NVME_CMD_RESERVATION_RELEASE = 0x15;
};

} // namespace fkernel