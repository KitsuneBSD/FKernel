#include <LibFK/Algorithms/Logging/log.h>
#include <LibFK/Utilities/memory.h>

#include <Kernel/Arch/x86_64/io.h>
#include <Kernel/Driver/Storage/Controllers/Ata/dma_strategy.h>
#include <Kernel/Driver/Async/dma_buffer.h>
#include <Kernel/Memory/VirtualMemory/virtual_memory_manager.h>
#include <Kernel/Scheduler/Core/scheduler.h>

namespace fkernel {

static constexpr uint8_t ATA_CMD_READ_DMA = 0xC8;
static constexpr uint8_t ATA_CMD_WRITE_DMA = 0xCA;
static constexpr uint8_t ATA_REG_STATUS = 7;
static constexpr uint8_t ATA_SR_BSY = 0x80;
static constexpr uint8_t ATA_SR_ERR = 0x01;

void DMAStrategy::wait_busy() {
    int timeout = 10000000;
    while ((inb(m_io_base + ATA_REG_STATUS) & ATA_SR_BSY) && --timeout > 0) {}
    if (timeout == 0) fk::algorithms::kwarn("DMA", "wait_busy timeout");
}

void DMAStrategy::prepare_transfer(uint64_t start_sector, uint8_t count, bool write) {
    outb(m_io_base + 6, (m_is_master ? 0xE0 : 0xF0) | ((start_sector >> 24) & 0x0F));
    outb(m_io_base + 2, count);
    outb(m_io_base + 3, (uint8_t)start_sector);
    outb(m_io_base + 4, (uint8_t)(start_sector >> 8));
    outb(m_io_base + 5, (uint8_t)(start_sector >> 16));
    outb(m_io_base + 7, write ? ATA_CMD_WRITE_DMA : ATA_CMD_READ_DMA);
}

fk::core::Result<size_t, fk::core::Error> DMAStrategy::read_sectors(uint64_t start_sector, size_t count, uint8_t* buffer) {
    if (count == 0 || !buffer) return fk::core::Error::InvalidParameter;

    fk::synchronization::ScopedLockIRQ lock(m_transfer_lock);

    if (!m_prdt_buffer.is_valid()) {
        TRY(m_prdt_buffer.allocate(4096));
    }
    PRD* prdt = reinterpret_cast<PRD*>(m_prdt_buffer.virtual_address());

    size_t total_done = 0;
    while (count > 0) {
        // ATA sector count register is 8-bit; 0 encodes 256.
        size_t chunk = (count > 256) ? 256 : count;
        size_t chunk_bytes = chunk * 512;
        uintptr_t cur_va = reinterpret_cast<uintptr_t>(buffer);
        size_t remaining = chunk_bytes;
        int prd_idx = 0;

        while (remaining > 0 && prd_idx < 1024) {
            uintptr_t page_off = cur_va % 4096;
            uintptr_t phys_base = VirtualMemoryManager::the().translate(cur_va - page_off);
            if (phys_base == 0) {
                fk::algorithms::kwarn("DMA", "read: translate failed for buffer %p", (void*)cur_va);
                return fk::core::Error::IOError;
            }
            size_t bytes_to_end = 4096 - page_off;
            size_t entry_size = (remaining < bytes_to_end) ? remaining : bytes_to_end;
            prdt[prd_idx].phys_addr  = (uint32_t)(phys_base + page_off);
            prdt[prd_idx].byte_count = (uint16_t)entry_size;
            prdt[prd_idx].reserved   = 0;
            prdt[prd_idx].last_entry = 0;
            remaining -= entry_size;
            cur_va    += entry_size;
            ++prd_idx;
        }
        prdt[prd_idx - 1].last_entry = 1;

        outl(m_bm_base + BM_PRDT_ADDR_REG, (uint32_t)m_prdt_buffer.physical_address().as_uintptr());
        outb(m_bm_base + BM_COMMAND_REG, BM_CMD_READ);
        outb(m_bm_base + BM_STATUS_REG, BM_STATUS_ERROR | BM_STATUS_INTERRUPT);

        wait_busy();
        prepare_transfer(start_sector, (uint8_t)(chunk == 256 ? 0 : chunk), false);
        outb(m_bm_base + BM_COMMAND_REG, BM_CMD_READ | BM_CMD_START);

        int timeout = 10000000;
        while (timeout-- > 0) {
            uint8_t bm_st = inb(m_bm_base + BM_STATUS_REG);
            uint8_t ata_st = inb(m_io_base + ATA_REG_STATUS);
            if (!(bm_st & BM_STATUS_ACTIVE)) {
                if (bm_st & BM_STATUS_ERROR) {
                    fk::algorithms::kwarn("DMA", "read: bus master error");
                    return fk::core::Error::IOError;
                }
                break;
            }
            if (!(ata_st & ATA_SR_BSY) && (bm_st & BM_STATUS_INTERRUPT)) break;
            SchedulerManager::the().yield();
        }
        outb(m_bm_base + BM_COMMAND_REG, 0x00);
        if (timeout == 0) {
            fk::algorithms::kwarn("DMA", "read: completion timeout");
            return fk::core::Error::DeviceError;
        }

        total_done   += chunk;
        start_sector += chunk;
        buffer       += chunk_bytes;
        count        -= chunk;
    }
    return total_done;
}

fk::core::Result<size_t, fk::core::Error> DMAStrategy::write_sectors(uint64_t start_sector, size_t count, const uint8_t* buffer) {
    if (count == 0 || !buffer) return fk::core::Error::InvalidParameter;

    fk::synchronization::ScopedLockIRQ lock(m_transfer_lock);

    if (!m_prdt_buffer.is_valid()) {
        TRY(m_prdt_buffer.allocate(4096));
    }
    PRD* prdt = reinterpret_cast<PRD*>(m_prdt_buffer.virtual_address());

    size_t total_done = 0;
    while (count > 0) {
        size_t chunk = (count > 256) ? 256 : count;
        size_t chunk_bytes = chunk * 512;
        uintptr_t cur_va = reinterpret_cast<uintptr_t>(buffer);
        size_t remaining = chunk_bytes;
        int prd_idx = 0;

        while (remaining > 0 && prd_idx < 1024) {
            uintptr_t page_off = cur_va % 4096;
            uintptr_t phys_base = VirtualMemoryManager::the().translate(cur_va - page_off);
            if (phys_base == 0) {
                fk::algorithms::kwarn("DMA", "write: translate failed for buffer %p", (void*)cur_va);
                return fk::core::Error::IOError;
            }
            size_t bytes_to_end = 4096 - page_off;
            size_t entry_size = (remaining < bytes_to_end) ? remaining : bytes_to_end;
            prdt[prd_idx].phys_addr  = (uint32_t)(phys_base + page_off);
            prdt[prd_idx].byte_count = (uint16_t)entry_size;
            prdt[prd_idx].reserved   = 0;
            prdt[prd_idx].last_entry = 0;
            remaining -= entry_size;
            cur_va    += entry_size;
            ++prd_idx;
        }
        prdt[prd_idx - 1].last_entry = 1;

        outl(m_bm_base + BM_PRDT_ADDR_REG, (uint32_t)m_prdt_buffer.physical_address().as_uintptr());
        outb(m_bm_base + BM_COMMAND_REG, 0);
        outb(m_bm_base + BM_STATUS_REG, BM_STATUS_ERROR | BM_STATUS_INTERRUPT);

        wait_busy();
        {
            uint8_t ata_st = inb(m_io_base + ATA_REG_STATUS);
            if (ata_st & ATA_SR_ERR) {
                fk::algorithms::kwarn("DMA", "write: ATA error before transfer, status=0x%02x", ata_st);
                return fk::core::Error::IOError;
            }
        }
        prepare_transfer(start_sector, (uint8_t)(chunk == 256 ? 0 : chunk), true);
        outb(m_bm_base + BM_COMMAND_REG, BM_CMD_START);

        int timeout = 10000000;
        while (timeout-- > 0) {
            uint8_t bm_st = inb(m_bm_base + BM_STATUS_REG);
            if (!(bm_st & BM_STATUS_ACTIVE)) {
                if (bm_st & BM_STATUS_ERROR) {
                    fk::algorithms::kwarn("DMA", "write: bus master error");
                    return fk::core::Error::IOError;
                }
                break;
            }
            SchedulerManager::the().yield();
        }
        outb(m_bm_base + BM_COMMAND_REG, 0x00);
        if (timeout == 0) {
            fk::algorithms::kwarn("DMA", "write: completion timeout");
            return fk::core::Error::DeviceError;
        }

        total_done   += chunk;
        start_sector += chunk;
        buffer       += chunk_bytes;
        count        -= chunk;
    }
    return total_done;
}

} // namespace fkernel