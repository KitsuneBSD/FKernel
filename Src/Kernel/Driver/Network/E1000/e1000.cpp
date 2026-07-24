#include <Kernel/Driver/Network/E1000/e1000.h>
#include <Kernel/Memory/Dma/dma_buffer.h>
#include <Kernel/Memory/memory_manager.h>
#include <Kernel/Hardware/Pci/pci.h>
#include <Kernel/Scheduler/scheduler.h>
#include <LibFK/Algorithms/log.h>
#include <LibFK/Core/assertions.h>
#include <LibFK/Memory/ref_ptr.h>
#include <LibFK/Utilities/memory.h>

namespace fkernel {

fk::RefPtr<E1000Controller> E1000Controller::create(const PciDevice& device) {
    auto controller_result = fk::make_ref<E1000Controller>(device);
    if (controller_result.is_error()) return nullptr;
    
    auto controller = controller_result.value();
    auto init_result = controller->initialize_hardware();
    if (init_result.is_error()) {
        fk::algorithms::kerror("E1000", "Failed to initialize hardware");
        return nullptr;
    }
    
    return controller;
}

E1000Controller::E1000Controller(const PciDevice& device) 
    : m_pci_device(device) {
    set_name("eth0");
}

E1000Controller::~E1000Controller() {}

fk::core::Result<void, fk::core::Error> E1000Controller::initialize_hardware() {
    // Map BAR0
    uint32_t bar0 = PciManager::the().read_config_dword(m_pci_device.address(), 0x10);
    uintptr_t mmio_phys = bar0 & ~0xF;
    
    MemoryManager::the().map_page(mmio_phys, mmio_phys, PageFlags::Present | PageFlags::Writable | PageFlags::CacheDisabled);
    m_mmio_base = reinterpret_cast<volatile uint8_t*>(mmio_phys);
    
    fk::algorithms::klog("E1000", "MMIO mapped at %p", (void*)mmio_phys);

    // Enable PCI Bus Master
    uint32_t command = PciManager::the().read_config_dword(m_pci_device.address(), 0x04);
    command |= 0x04; 
    PciManager::the().write_config_dword(m_pci_device.address(), 0x04, command);

    read_mac_address();
    fk::algorithms::klog("E1000", "MAC Address: %s", m_mac.to_string().c_str());

    initialize_rx();
    initialize_tx();

    // Enable interrupts (combined mask: 0x1F6DC with bit 2 cleared)
    write_command(REG_IMASK, 0x1F6DC & ~static_cast<uint32_t>(4));
    read_command(0xc0);

    fk::algorithms::klog("E1000", "Initialized successfully");
    return {};
}

void E1000Controller::write_command(uint16_t addr, uint32_t val) {
    *reinterpret_cast<volatile uint32_t*>(m_mmio_base + addr) = val;
}

uint32_t E1000Controller::read_command(uint16_t addr) {
    return *reinterpret_cast<volatile uint32_t*>(m_mmio_base + addr);
}

void E1000Controller::read_mac_address() {
    uint32_t temp = read_command(0x5400); // RAL
    m_mac.address[0] = temp & 0xFF;
    m_mac.address[1] = (temp >> 8) & 0xFF;
    m_mac.address[2] = (temp >> 16) & 0xFF;
    m_mac.address[3] = (temp >> 24) & 0xFF;
    temp = read_command(0x5404); // RAH
    m_mac.address[4] = temp & 0xFF;
    m_mac.address[5] = (temp >> 8) & 0xFF;
}

void E1000Controller::initialize_rx() {
    auto ring_result = dma_alloc_buffer(sizeof(e1000_rx_desc) * 128);
    if (ring_result.is_error()) {
        fk::algorithms::kerror("E1000", "Failed to allocate RX descriptor ring");
        return;
    }
    m_rx_ring = ring_result.value();
    m_rx_descs = reinterpret_cast<e1000_rx_desc*>(m_rx_ring.vaddr);

    for (int i = 0; i < 128; i++) {
        auto buf_result = dma_alloc_buffer(2048);
        if (buf_result.is_error()) {
            fk::algorithms::kerror("E1000", "Failed to allocate RX buffer %d", i);
            return;
        }
        m_rx_buffers[i] = buf_result.value();
        m_rx_descs[i].addr = m_rx_buffers[i].phys;
        m_rx_descs[i].status = 0;
    }

    write_command(REG_RXADDRL, (uint32_t)m_rx_ring.phys);
    write_command(REG_RXADDRH, (uint32_t)(m_rx_ring.phys >> 32));
    write_command(REG_RXLEN, 128 * sizeof(e1000_rx_desc));
    write_command(REG_RXHEAD, 0);
    write_command(REG_RXTAIL, 127);

    uint32_t rctrl = (1 << 1) | // EN (Receiver Enable)
                     (1 << 4) | // Multicast Promiscuous
                     (1 << 15) | // BAM (Broadcast Accept Mode)
                     (0 << 16) | // RCTL_BSIZE_2048
                     (1 << 26);  // SECRC (Strip Ethernet CRC)
    write_command(REG_RCTRL, rctrl);
}

void E1000Controller::initialize_tx() {
    auto ring_result = dma_alloc_buffer(sizeof(e1000_tx_desc) * 128);
    if (ring_result.is_error()) {
        fk::algorithms::kerror("E1000", "Failed to allocate TX descriptor ring");
        return;
    }
    m_tx_ring = ring_result.value();
    m_tx_descs = reinterpret_cast<e1000_tx_desc*>(m_tx_ring.vaddr);

    for (int i = 0; i < 128; i++) {
        auto buf_result = dma_alloc_buffer(2048);
        if (buf_result.is_error()) {
            fk::algorithms::kerror("E1000", "Failed to allocate TX buffer %d", i);
            return;
        }
        m_tx_buffers[i] = buf_result.value();
        m_tx_descs[i].addr = m_tx_buffers[i].phys;
        m_tx_descs[i].status = 0;
    }

    write_command(REG_TXADDRL, (uint32_t)m_tx_ring.phys);
    write_command(REG_TXADDRH, (uint32_t)(m_tx_ring.phys >> 32));
    write_command(REG_TXLEN, 128 * sizeof(e1000_tx_desc));
    write_command(REG_TXHEAD, 0);
    write_command(REG_TXTAIL, 0);

    uint32_t tctrl = (1 << 1) | // EN (Transmit Enable)
                     (1 << 3) | // PSP (Pad Short Packets)
                     (0x0F << 4) | // CT (Collision Threshold)
                     (0x40 << 12); // COLD (Collision Distance)
    write_command(REG_TCTRL, tctrl);
}

fk::core::Result<void, fk::core::Error> E1000Controller::send_packet(const uint8_t* data, size_t size) {
    if (size > 2048) {
        fk::algorithms::kwarn("E1000", "Attempted to send packet too large (%zu bytes)", size);
        return fk::core::Error::InvalidParameter;
    }

    m_tx_descs[m_tx_current].len = (uint16_t)size;
    m_tx_descs[m_tx_current].status = 0;
    m_tx_descs[m_tx_current].lower_setup = (1 << 3) | (1 << 0) | (1 << 1); // RS, EOP, IFCS
    fk::memory::copy(m_tx_buffers[m_tx_current].vaddr, data, size);

    uint16_t old_tx = m_tx_current;
    m_tx_current = (m_tx_current + 1) % 128;
    write_command(REG_TXTAIL, m_tx_current);

    fk::algorithms::kdebug("E1000", "Packet sent: %zu bytes (TX Tail: %u)", size, m_tx_current);

    // Polling for completion
    int timeout = 1000000;
    while (!(m_tx_descs[old_tx].status & 0xF) && timeout > 0) {
        timeout--;
        if (timeout % 1000 == 0) {
            SchedulerManager::the().yield();
        }
    }
    
    if (timeout == 0) {
        fk::algorithms::kwarn("E1000", "Packet transmission timeout");
        return fk::core::Error::DeviceError;
    }

    return {};
}

fk::core::Result<size_t, fk::core::Error> E1000Controller::receive_packet(uint8_t* buffer, size_t max_size) {
    if (!(m_rx_descs[m_rx_current].status & 0x01)) return fk::core::Error::DeviceError;

    size_t size = m_rx_descs[m_rx_current].len;
    fk::algorithms::kdebug("E1000", "Packet received: %zu bytes (RX Head: %u)", size, m_rx_current);

    if (size > max_size) size = max_size;

    fk::memory::copy(buffer, m_rx_buffers[m_rx_current].vaddr, size);

    m_rx_descs[m_rx_current].status = 0;
    uint16_t old_rx = m_rx_current;
    m_rx_current = (m_rx_current + 1) % 128;
    write_command(REG_RXTAIL, old_rx);

    return size;
}

void E1000Controller::probe() {
    fk::algorithms::klog("E1000", "Probing...");
}

fk::core::Result<size_t, fk::core::Error> E1000Controller::read(uint64_t, size_t size, uint8_t* buffer) {
    return receive_packet(buffer, size);
}

fk::core::Result<size_t, fk::core::Error> E1000Controller::write(uint64_t, size_t size, const uint8_t* buffer) {
    auto res = send_packet(buffer, size);
    if (res.is_error()) return res.error();
    return size;
}

} // namespace fkernel
