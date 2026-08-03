#pragma once

#include <Kernel/Driver/Storage/Controllers/Ata/ata_transfer_strategy.h>
#include <Kernel/Memory/Dma/dma_buffer.h>
#include <LibFK/Synchronization/spinlock.h>
#include <LibFK/Types/types.h>

namespace fkernel {

class DMAStrategy : public ATATransferStrategy {
    uint16_t m_io_base;
    uint16_t m_bm_base; // Bus Master base address
    bool m_is_master;
    fk::synchronization::Spinlock m_transfer_lock;
    DmaBuffer m_prdt_buffer;

    // Bus Master IDE Registers (relative to m_bm_base)
    static constexpr uint8_t BM_COMMAND_REG = 0x0;
    static constexpr uint8_t BM_STATUS_REG = 0x2;
    static constexpr uint8_t BM_PRDT_ADDR_REG = 0x4;

    // Bus Master Command Register Bits
    static constexpr uint8_t BM_CMD_START = 0x1;
    static constexpr uint8_t BM_CMD_READ = 0x8; // Read from drive (Write to memory)

    // Bus Master Status Register Bits
    static constexpr uint8_t BM_STATUS_ACTIVE = 0x1;
    static constexpr uint8_t BM_STATUS_ERROR = 0x2;
    static constexpr uint8_t BM_STATUS_INTERRUPT = 0x4;

public:
    DMAStrategy(uint16_t io_base, uint16_t bm_base, bool is_master)
        : m_io_base(io_base), m_bm_base(bm_base), m_is_master(is_master) {}

    virtual fk::core::Result<size_t, fk::core::Error>
    read_sectors(uint64_t start_sector, size_t count, uint8_t *buffer) override;

    virtual fk::core::Result<size_t, fk::core::Error>
    write_sectors(uint64_t start_sector, size_t count, const uint8_t *buffer) override;

    virtual const char *name() const override { return "DMA"; }

private:
    struct PRD {
        uint32_t phys_addr;
        uint16_t byte_count;
        uint16_t reserved : 15;
        uint16_t last_entry : 1;
    } __attribute__((packed));

    void wait_busy();
    void prepare_transfer(uint64_t start_sector, uint8_t count, bool write);
};

} // namespace fkernel
