#include <Kernel/Driver/Mouse/ps2_mouse.h>
#include <Kernel/Arch/x86_64/io.h>
#include <LibFK/Algorithms/log.h>
#include <LibFK/Synchronization/spinlock.h>

static constexpr uint16_t PS2_DATA_PORT   = 0x60;
static constexpr uint16_t PS2_STATUS_PORT = 0x64;
static constexpr uint8_t  PS2_CMD_ENABLE_MOUSE   = 0xA8;
static constexpr uint8_t  PS2_CMD_READ_CONFIG     = 0x20;
static constexpr uint8_t  PS2_CMD_WRITE_CONFIG    = 0x60;
static constexpr uint8_t  PS2_CMD_SEND_TO_MOUSE   = 0xD4;
static constexpr uint8_t  MOUSE_CMD_SET_DEFAULTS  = 0xF6;
static constexpr uint8_t  MOUSE_CMD_ENABLE_STREAM = 0xF4;

PS2Mouse& PS2Mouse::the() {
    static PS2Mouse instance;
    return instance;
}

PS2Mouse::PS2Mouse() {
    set_name("mouse");
}

static void ps2_wait_write() {
    for (int i = 0; i < 100000; ++i) {
        if (!(inb(PS2_STATUS_PORT) & 0x02)) return;
    }
    fk::algorithms::kwarn("MOUSE", "ps2_wait_write timeout");
}

static void ps2_wait_read() {
    for (int i = 0; i < 100000; ++i) {
        if (inb(PS2_STATUS_PORT) & 0x01) return;
    }
    fk::algorithms::kwarn("MOUSE", "ps2_wait_read timeout");
}

void PS2Mouse::send_command(uint8_t cmd) {
    ps2_wait_write();
    outb(PS2_STATUS_PORT, PS2_CMD_SEND_TO_MOUSE);
    ps2_wait_write();
    outb(PS2_DATA_PORT, cmd);
    ps2_wait_read();
    inb(PS2_DATA_PORT); // consume ACK
}

void PS2Mouse::initialize() {
    if (m_hw_initialized) return;

    // Enable mouse port
    ps2_wait_write();
    outb(PS2_STATUS_PORT, PS2_CMD_ENABLE_MOUSE);

    // Read controller config, enable mouse interrupt (bit 1)
    ps2_wait_write();
    outb(PS2_STATUS_PORT, PS2_CMD_READ_CONFIG);
    ps2_wait_read();
    uint8_t config = inb(PS2_DATA_PORT);
    config |= 0x02; // enable mouse interrupt

    ps2_wait_write();
    outb(PS2_STATUS_PORT, PS2_CMD_WRITE_CONFIG);
    ps2_wait_write();
    outb(PS2_DATA_PORT, config);

    send_command(MOUSE_CMD_SET_DEFAULTS);
    send_command(MOUSE_CMD_ENABLE_STREAM);

    m_hw_initialized = true;
    fk::algorithms::klog("PS2MOUSE", "PS/2 Mouse initialized");
}

fk::core::Result<void, fk::core::Error> PS2Mouse::on_open() {
    initialize();
    return {};
}

void PS2Mouse::irq_handler() {
    if (!m_hw_initialized) { inb(PS2_DATA_PORT); return; } // drain spurious byte
    uint8_t data = inb(PS2_DATA_PORT);
    m_packet[m_packet_byte++] = data;

    if (m_packet_byte == 1) {
        m_packet_size = (data & 0x20) ? 4 : 3;
    }

    if (m_packet_byte == m_packet_size) {
        m_packet_byte = 0;
        process_packet();
    }
}

void PS2Mouse::process_packet() {
    uint8_t status = m_packet[0];
    if (!(status & 0x08)) return; // bit 3 must be 1 (sync)

    MouseEvent ev;
    ev.buttons = status & 0x07;
    ev.dx = (int8_t)m_packet[1];
    ev.dy = -(int8_t)m_packet[2]; // Y is inverted in PS/2
    ev.scroll = (m_packet_size == 4) ? (int8_t)m_packet[3] : 0;

    fk::synchronization::ScopedLock lock(m_lock);
    if (m_events.size() < MOUSE_BUFFER_SIZE)
        m_events.push_back(ev);
}

fk::core::Result<size_t, fk::core::Error> PS2Mouse::read(
    [[maybe_unused]] uint64_t offset, size_t size, uint8_t* buf) {
    fk::synchronization::ScopedLock lock(m_lock);
    if (m_events.is_empty()) return (size_t)0;
    size_t events_to_read = size / sizeof(MouseEvent);
    size_t available = m_events.size();
    size_t count = events_to_read < available ? events_to_read : available;
    for (size_t i = 0; i < count; ++i) {
        MouseEvent* out = reinterpret_cast<MouseEvent*>(buf) + i;
        *out = m_events[i];
    }
    for (size_t i = count; i < available; ++i)
        m_events[i - count] = m_events[i];
    for (size_t i = 0; i < count; ++i) m_events.pop_back();
    return count * sizeof(MouseEvent);
}
