#include <LibFK/Algorithms/Logging/log.h>
#include <LibFK/Core/assertions.h>
#include <LibFK/Memory/Pointers/ref_ptr.h>
#include <LibFK/Utilities/memory.h>
#include <LibFK/Synchronization/interrupt_disabler.h>

#include <Kernel/Driver/Network/E1000/e1000.h>
#include <Kernel/Arch/x86_64/Interrupt/interrupt_controller.h>
#include <Kernel/Arch/x86_64/Interrupt/interrupt_types.h>
#include <Kernel/Arch/x86_64/Interrupt/HardwareInterrupts/tick_manager.h>
#include <Kernel/Driver/Async/dma_buffer.h>
#include <Kernel/Memory/memory_manager.h>
#include <Kernel/Hardware/Buses/Pci/pci.h>
#include <Kernel/Scheduler/Core/scheduler.h>

namespace fkernel {

E1000Controller* E1000Controller::s_instances[32] = {};

static void e1000_isr(uint8_t vector, InterruptFrame*) {
    uint8_t idx = vector - 32;
    if (idx < 32 && E1000Controller::s_instances[idx]) {
        E1000Controller::s_instances[idx]->handle_interrupt();
    }
}

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

E1000Controller::~E1000Controller() {
    if (m_irq_line > 0 && m_irq_line < 32) {
        s_instances[m_irq_line] = nullptr;
    }
}

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

    // Read IRQ line from PCI config (offset 0x3C, low byte = Interrupt Line)
    m_irq_line = PciManager::the().read_config_byte(m_pci_device.address(), 0x3C);

    // Enable interrupts: TX Descriptor Written Back + RX Timer + Link Status + others
    // Bit 0: TXDW, Bit 7: RXT0, Bit 2: LSC, others as needed
    uint32_t imask = ICR_TXDW | ICR_RXT0 | ICR_LSC | ICR_TXQE;
    write_command(REG_IMASK, imask);

    // Clear any pending interrupts
    read_command(REG_ICR);

    // Register ISR if IRQ is valid (legacy PCI IRQ = 1-15)
    if (m_irq_line > 0 && m_irq_line < 32) {
        uint8_t vector = static_cast<uint8_t>(m_irq_line + 32);
        s_instances[m_irq_line] = this;
        InterruptController::the().register_interrupt(e1000_isr, vector);
        fk::algorithms::klog("E1000", "Registered ISR for IRQ %u (vector %u)", m_irq_line, vector);
    } else {
        fk::algorithms::kwarn("E1000", "No valid IRQ line (%u), falling back to polling", m_irq_line);
    }

    fk::algorithms::klog("E1000", "Initialized successfully");
    return {};
}

void E1000Controller::handle_interrupt() {
    uint32_t icr = read_command(REG_ICR);

    if (icr & ICR_TXDW) {
        m_tx_done = true;
        Task* waiter = m_tx_wait_task;
        if (waiter) {
            m_tx_wait_task = nullptr;
            SchedulerManager::the().wake_task(waiter);
        }
    }

    if (icr & ICR_LSC) {
        uint32_t status = read_command(REG_STATUS);
        fk::algorithms::klog("E1000", "Link status changed: %s",
                             (status & (1 << 1)) ? "Up" : "Down");
    }
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
    if (m_rx_ring.allocate(sizeof(e1000_rx_desc) * 128).is_error()) {
        fk::algorithms::kerror("E1000", "Failed to allocate RX descriptor ring");
        return;
    }
    m_rx_descs = reinterpret_cast<e1000_rx_desc*>(m_rx_ring.virtual_address());

    for (int i = 0; i < 128; i++) {
        if (m_rx_buffers[i].allocate(2048).is_error()) {
            fk::algorithms::kerror("E1000", "Failed to allocate RX buffer %d", i);
            return;
        }
        m_rx_descs[i].addr = m_rx_buffers[i].physical_address().as_uintptr();
        m_rx_descs[i].status = 0;
    }

    write_command(REG_RXADDRL, (uint32_t)m_rx_ring.physical_address().as_uintptr());
    write_command(REG_RXADDRH, (uint32_t)(m_rx_ring.physical_address().as_uintptr() >> 32));
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
    if (m_tx_ring.allocate(sizeof(e1000_tx_desc) * 128).is_error()) {
        fk::algorithms::kerror("E1000", "Failed to allocate TX descriptor ring");
        return;
    }
    m_tx_descs = reinterpret_cast<e1000_tx_desc*>(m_tx_ring.virtual_address());

    for (int i = 0; i < 128; i++) {
        if (m_tx_buffers[i].allocate(2048).is_error()) {
            fk::algorithms::kerror("E1000", "Failed to allocate TX buffer %d", i);
            return;
        }
        m_tx_descs[i].addr = m_tx_buffers[i].physical_address().as_uintptr();
        m_tx_descs[i].status = 0;
    }

    write_command(REG_TXADDRL, (uint32_t)m_tx_ring.physical_address().as_uintptr());
    write_command(REG_TXADDRH, (uint32_t)(m_tx_ring.physical_address().as_uintptr() >> 32));
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
    fk::memory::copy(m_tx_buffers[m_tx_current].virtual_address(), data, size);

    uint16_t old_tx = m_tx_current;
    m_tx_current = (m_tx_current + 1) % 128;

    // Clear completion flag before ringing the bell
    m_tx_done = false;

    // Ring the doorbell — hardware starts DMA
    write_command(REG_TXTAIL, m_tx_current);

    fk::algorithms::kdebug("E1000", "Packet submitted: %zu bytes (TX Tail: %u)", size, m_tx_current);

    // Wait for TX completion interrupt (or poll as fallback)
    constexpr int MAX_TICKS = 50; // 50 ticks (~500ms at 100Hz)
    uint64_t deadline = TickManager::the().get_ticks() + MAX_TICKS;

    if (m_irq_line > 0 && m_irq_line < 32) {
        // Interrupt-driven path: sleep until ISR wakes us
        while (!m_tx_done) {
            fk::synchronization::ScopedInterruptDisabler intr;
            if (m_tx_done) break;
            m_tx_wait_task = SchedulerManager::the().current();
            SchedulerManager::the().block_current();
            SchedulerManager::the().schedule();
        }
        m_tx_wait_task = nullptr;
    } else {
        // Fallback: polling with yield (no IRQ registered)
        while (!(m_tx_descs[old_tx].status & 0xF)) {
            if (TickManager::the().get_ticks() >= deadline) {
                fk::algorithms::kwarn("E1000", "Packet transmission timeout (polling)");
                return fk::core::Error::DeviceError;
            }
            SchedulerManager::the().yield();
        }
        m_tx_done = true;
    }

    // Verify hardware actually completed the descriptor
    if (!(m_tx_descs[old_tx].status & 0xF)) {
        fk::algorithms::kwarn("E1000", "TX descriptor not completed after wake");
        return fk::core::Error::DeviceError;
    }

    return {};
}

fk::core::Result<size_t, fk::core::Error> E1000Controller::receive_packet(uint8_t* buffer, size_t max_size) {
    if (!(m_rx_descs[m_rx_current].status & 0x01)) return fk::core::Error::DeviceError;

    size_t size = m_rx_descs[m_rx_current].len;
    fk::algorithms::kdebug("E1000", "Packet received: %zu bytes (RX Head: %u)", size, m_rx_current);

    if (size > max_size) size = max_size;

    fk::memory::copy(buffer, m_rx_buffers[m_rx_current].virtual_address(), size);

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
