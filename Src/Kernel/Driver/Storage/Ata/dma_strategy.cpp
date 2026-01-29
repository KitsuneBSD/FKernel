#include <Kernel/Arch/x86_64/io.h>
#include <Kernel/Driver/Storage/Ata/dma_strategy.h>
#include <Kernel/Memory/PhysicalMemory/physical_memory_manager.h>
#include <Kernel/Memory/VirtualMemory/virtual_memory_manager.h>
#include <Kernel/Scheduler/scheduler.h>
#include <LibFK/Algorithms/log.h>
#include <LibC/string.h>

namespace fkernel {

static constexpr uint8_t ATA_CMD_READ_DMA = 0xC8;
static constexpr uint8_t ATA_CMD_WRITE_DMA = 0xCA;
static constexpr uint8_t ATA_REG_STATUS = 7;
static constexpr uint8_t ATA_SR_BSY = 0x80;

void DMAStrategy::wait_busy() {
    while (inb(m_io_base + ATA_REG_STATUS) & ATA_SR_BSY);
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
    if (count > 256) return fk::core::Error::NotImplemented; // Limit to 256 sectors for now

    // 1. Setup PRDT
    // We need a physical address for the PRDT itself.
    // For now, let's use a static or pre-allocated page.
    static uintptr_t prdt_phys = 0;
    if (prdt_phys == 0) {
        prdt_phys = PhysicalMemoryManager::the().alloc_page();
    }
    PRD* prdt = reinterpret_cast<PRD*>(prdt_phys); // Assuming kernel identity mapping for this page

    size_t total_bytes = count * 512;
    uintptr_t current_buffer_virt = reinterpret_cast<uintptr_t>(buffer);
    size_t remaining_bytes = total_bytes;
    int prd_idx = 0;

    // Build PRDT by translating buffer pages to physical addresses
    while (remaining_bytes > 0 && prd_idx < 1024) {
        uintptr_t page_offset = current_buffer_virt % 4096;
        uintptr_t page_base = current_buffer_virt - page_offset;
        
        uintptr_t phys_base = VirtualMemoryManager::the().translate(page_base);
        if (phys_base == 0) return fk::core::Error::IOError;
        
        uintptr_t phys = phys_base + page_offset;

        size_t bytes_to_page_end = 4096 - page_offset;
        size_t chunk_size = (remaining_bytes < bytes_to_page_end) ? remaining_bytes : bytes_to_page_end;

        prdt[prd_idx].phys_addr = (uint32_t)phys;
        prdt[prd_idx].byte_count = (uint16_t)chunk_size;
        prdt[prd_idx].reserved = 0;
        prdt[prd_idx].last_entry = 0;

        remaining_bytes -= chunk_size;
        current_buffer_virt += chunk_size;
        prd_idx++;
    }
    prdt[prd_idx - 1].last_entry = 1;

    // 2. Setup Bus Master
    outl(m_bm_base + BM_PRDT_ADDR_REG, (uint32_t)prdt_phys);
    
    // Set direction (Read from drive = 1 in Bit 3 of BM Command)
    outb(m_bm_base + BM_COMMAND_REG, BM_CMD_READ);
    
    // Clear Error and Interrupt bits in Status Register
    outb(m_bm_base + BM_STATUS_REG, BM_STATUS_ERROR | BM_STATUS_INTERRUPT);

    // 3. Command Drive
    wait_busy();
    prepare_transfer(start_sector, (uint8_t)(count == 256 ? 0 : count), false);

    // 4. Start Bus Master
    outb(m_bm_base + BM_COMMAND_REG, BM_CMD_READ | BM_CMD_START);

    // 5. Wait for completion (Polling status for now, ideally IRQ based)
    while (true) {
        uint8_t status = inb(m_bm_base + BM_STATUS_REG);
        uint8_t drive_status = inb(m_io_base + ATA_REG_STATUS);

        if (!(status & BM_STATUS_ACTIVE)) {
            if (status & BM_STATUS_ERROR) return fk::core::Error::IOError;
            break;
        }
        
        if (!(drive_status & ATA_SR_BSY) && (status & BM_STATUS_INTERRUPT)) {
            break;
        }
        
        SchedulerManager::the().yield();
    }

    // 6. Stop Bus Master
    outb(m_bm_base + BM_COMMAND_REG, 0x00);

    return count;
}

fk::core::Result<size_t, fk::core::Error> DMAStrategy::write_sectors(uint64_t start_sector, size_t count, const uint8_t* buffer) {
    if (count == 0 || !buffer) return fk::core::Error::InvalidParameter;
    if (count > 256) return fk::core::Error::NotImplemented;

    static uintptr_t prdt_phys = 0;
    if (prdt_phys == 0) prdt_phys = PhysicalMemoryManager::the().alloc_page();
    PRD* prdt = reinterpret_cast<PRD*>(prdt_phys);

    size_t total_bytes = count * 512;
    uintptr_t current_buffer_virt = reinterpret_cast<uintptr_t>(buffer);
    size_t remaining_bytes = total_bytes;
    int prd_idx = 0;

    while (remaining_bytes > 0 && prd_idx < 1024) {
        uintptr_t page_offset = current_buffer_virt % 4096;
        uintptr_t page_base = current_buffer_virt - page_offset;

        uintptr_t phys_base = VirtualMemoryManager::the().translate(page_base);
        if (phys_base == 0) return fk::core::Error::IOError;

        uintptr_t phys = phys_base + page_offset;
        size_t bytes_to_page_end = 4096 - page_offset;
        size_t chunk_size = (remaining_bytes < bytes_to_page_end) ? remaining_bytes : bytes_to_page_end;
        prdt[prd_idx].phys_addr = (uint32_t)phys;
        prdt[prd_idx].byte_count = (uint16_t)chunk_size;
        prdt[prd_idx].reserved = 0;
        prdt[prd_idx].last_entry = 0;
        remaining_bytes -= chunk_size;
        current_buffer_virt += chunk_size;
        prd_idx++;
    }
    prdt[prd_idx - 1].last_entry = 1;

    outl(m_bm_base + BM_PRDT_ADDR_REG, (uint32_t)prdt_phys);
    outb(m_bm_base + BM_COMMAND_REG, 0); // Write to drive = 0 in Bit 3
    outb(m_bm_base + BM_STATUS_REG, BM_STATUS_ERROR | BM_STATUS_INTERRUPT);

    wait_busy();
    prepare_transfer(start_sector, (uint8_t)(count == 256 ? 0 : count), true);

    outb(m_bm_base + BM_COMMAND_REG, BM_CMD_START);

    while (true) {
        uint8_t status = inb(m_bm_base + BM_STATUS_REG);
        if (!(status & BM_STATUS_ACTIVE)) break;
        SchedulerManager::the().yield();
    }

    outb(m_bm_base + BM_COMMAND_REG, 0x00);
    return count;
}

} // namespace fkernel