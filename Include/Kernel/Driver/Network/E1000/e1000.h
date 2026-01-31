#pragma once

#include <Kernel/Driver/Device/driver_manager.h>
#include <Kernel/Driver/Network/network_device.h>
#include <Kernel/Hardware/Pci/pci_device.h>
#include <LibFK/Types/types.h>

namespace fkernel {

class E1000Controller final : public Driver, public NetworkDevice {
public:
    static fk::RefPtr<E1000Controller> create(const PciDevice& device);
    
    virtual ~E1000Controller() override;

    // Driver interface
    virtual const char* name() const override { return "E1000Controller"; }
    virtual void probe() override;

    // NetworkDevice interface
    virtual fk::core::Result<void, fk::core::Error> send_packet(const uint8_t* data, size_t size) override;
    virtual fk::core::Result<size_t, fk::core::Error> receive_packet(uint8_t* buffer, size_t max_size) override;
    virtual MACAddress mac_address() const override { return m_mac; }

    // Node interface
    virtual fk::core::Result<size_t, fk::core::Error> read(uint64_t offset, size_t size, uint8_t* buffer) override;
    virtual fk::core::Result<size_t, fk::core::Error> write(uint64_t offset, size_t size, const uint8_t* buffer) override;
    virtual size_t size() const override { return 0; }

public:
    E1000Controller(const PciDevice& device);

private:
    struct e1000_rx_desc {
        uint64_t addr;
        uint16_t len;
        uint16_t checksum;
        uint8_t status;
        uint8_t errors;
        uint16_t special;
    } __attribute__((packed));

    struct e1000_tx_desc {
        uint64_t addr;
        uint16_t len;
        uint8_t lower_setup;
        uint8_t upper_setup;
        uint8_t status;
        uint8_t css;
        uint16_t special;
    } __attribute__((packed));

    fk::core::Result<void, fk::core::Error> initialize_hardware();
    void write_command(uint16_t addr, uint32_t val);
    uint32_t read_command(uint16_t addr);
    void read_mac_address();
    
    void initialize_rx();
    void initialize_tx();

    volatile uint8_t* m_mmio_base{nullptr};
    PciDevice m_pci_device;
    MACAddress m_mac;

    // Queues
    uint8_t* m_rx_buffer[128];
    e1000_rx_desc* m_rx_descs{nullptr};
    uint16_t m_rx_current{0};

    uint8_t* m_tx_buffer[128];
    e1000_tx_desc* m_tx_descs{nullptr};
    uint16_t m_tx_current{0};

    // Registers
    static constexpr uint16_t REG_CTRL = 0x0000;
    static constexpr uint16_t REG_STATUS = 0x0008;
    static constexpr uint16_t REG_EEPROM = 0x0014;
    static constexpr uint16_t REG_IMASK = 0x00D0;
    static constexpr uint16_t REG_RCTRL = 0x0100;
    static constexpr uint16_t REG_TCTRL = 0x0400;
    static constexpr uint16_t REG_RXADDRL = 0x2800;
    static constexpr uint16_t REG_RXADDRH = 0x2804;
    static constexpr uint16_t REG_RXLEN = 0x2808;
    static constexpr uint16_t REG_RXHEAD = 0x2810;
    static constexpr uint16_t REG_RXTAIL = 0x2818;
    static constexpr uint16_t REG_TXADDRL = 0x3800;
    static constexpr uint16_t REG_TXADDRH = 0x3804;
    static constexpr uint16_t REG_TXLEN = 0x3808;
    static constexpr uint16_t REG_TXHEAD = 0x3810;
    static constexpr uint16_t REG_TXTAIL = 0x3818;
    static constexpr uint16_t REG_RDTR = 0x2820; // RX Delay Timer
    static constexpr uint16_t REG_RADV = 0x282C; // RX ADV
    static constexpr uint16_t REG_RSRPD = 0x2C00; // RX Small Packet Detect
};

} // namespace fkernel
