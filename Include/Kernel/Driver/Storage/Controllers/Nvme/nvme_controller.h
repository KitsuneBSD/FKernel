#pragma once

#include <Kernel/Driver/Device/driver_manager.h>
#include <Kernel/Driver/Storage/Interfaces/storage_device.h>
#include <Kernel/Driver/Storage/Controllers/Nvme/nvme_command.h>
#include <Kernel/Driver/Storage/Controllers/Nvme/nvme_completion.h>
#include <Kernel/Driver/Storage/Controllers/Nvme/nvme_namespace.h>
#include <Kernel/Driver/Storage/Controllers/Nvme/nvme_queue_pair.h>
#include <Kernel/Hardware/Buses/Pci/pci_device.h>
#include <LibFK/Types/types.h>

namespace fkernel {

class NVMeController final : public Driver, public StorageDevice {
public:
    static fk::RefPtr<NVMeController> create(const PciDevice& device);

    virtual ~NVMeController() override;

    virtual const char* name() const override { return "NVMeController"; }
    virtual void probe() override;

    virtual fk::core::Result<size_t, fk::core::Error>
    read_sectors(uint64_t start_sector, size_t count, uint8_t *buffer) override;
    virtual fk::core::Result<size_t, fk::core::Error>
    write_sectors(uint64_t start_sector, size_t count, const uint8_t *buffer) override;

    virtual SectorSize sector_size() const override {
        return SectorSize(m_namespaces.is_empty() ? 512u : m_namespaces[0].block_size);
    }
    virtual SectorCount sector_count() const override;

    virtual fk::core::Result<size_t, fk::core::Error>
    read(uint64_t offset, size_t size, uint8_t* buffer) override;
    virtual fk::core::Result<size_t, fk::core::Error>
    write(uint64_t offset, size_t size, const uint8_t* buffer) override;
    virtual size_t size() const override;
    virtual bool is_block_device() const override { return true; }

public:
    NVMeController(const PciDevice& device);

    fk::core::Result<void, fk::core::Error> initialize_controller();
    fk::core::Result<void, fk::core::Error> reset_controller();
    fk::core::Result<void, fk::core::Error> identify_controller();
    fk::core::Result<void, fk::core::Error> configure_admin_queue();
    fk::core::Result<void, fk::core::Error> create_io_queues();
    void scan_namespaces();

    fk::core::Result<void, fk::core::Error> submit_command(NvmeQueuePair& queue, NvmeCommand& cmd);

    NvmeQueuePair m_admin_queue;
    NvmeQueuePair m_io_queue;

    volatile uint8_t* m_controller_regs{nullptr};
    uint64_t m_controller_capabilities{0};
    uint32_t m_version{0};

    PciDevice m_pci_device;
    bool m_initialized{false};

    static constexpr uint64_t NVME_CAP  = 0x0000;
    static constexpr uint64_t NVME_VS   = 0x0008;
    static constexpr uint64_t NVME_INTMS = 0x000C;
    static constexpr uint64_t NVME_INTMC = 0x0010;
    static constexpr uint64_t NVME_CC   = 0x0014;
    static constexpr uint64_t NVME_CSTS = 0x001C;
    static constexpr uint64_t NVME_NSSR = 0x0020;
    static constexpr uint64_t NVME_AQA  = 0x0024;
    static constexpr uint64_t NVME_ASQ  = 0x0028;
    static constexpr uint64_t NVME_ACQ  = 0x0030;

    uint32_t m_doorbell_stride{0};

    static constexpr uint8_t NVME_PCI_BAR = 0x00;

    static constexpr uint8_t NVME_CMD_CREATE_IO_SQ = 0x01;
    static constexpr uint8_t NVME_CMD_CREATE_IO_CQ = 0x05;
    static constexpr uint8_t NVME_CMD_IDENTIFY     = 0x06;
    static constexpr uint8_t NVME_CMD_WRITE        = 0x01;
    static constexpr uint8_t NVME_CMD_READ         = 0x02;

    fk::containers::Vector<NvmeNamespace> m_namespaces;
};

} // namespace fkernel
