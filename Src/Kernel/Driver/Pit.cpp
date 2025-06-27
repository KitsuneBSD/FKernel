#include <Kernel/Arch/x86_64/Hw/Io.h>
#include <Kernel/Driver/Pit.h>
#include <LibFK/Log.h>

constexpr LibC::uint32_t PIT_FREQUENCY = 1193182; // Frequência do clock do PIT (Hz)
constexpr LibC::uint32_t PIT_CHANNEL0_PORT = 0x40;
constexpr LibC::uint32_t PIT_COMMAND_PORT = 0x43;

constexpr LibC::uint8_t PIT_CMD_CHANNEL0 = 0b00 << 6;
constexpr LibC::uint8_t PIT_CMD_ACCESS_LOHI = 0b11 << 4;
constexpr LibC::uint8_t PIT_CMD_MODE2 = 0b010 << 1;
constexpr LibC::uint8_t PIT_CMD_BINARY = 0b0;

constexpr LibC::uint8_t PIT_COMMAND = PIT_CMD_CHANNEL0 | PIT_CMD_ACCESS_LOHI | PIT_CMD_MODE2 | PIT_CMD_BINARY;

void Pit::initialize(LibC::uint32_t frequency) noexcept
{
    if (frequency == 0) {
        Log(LogLevel::ERROR, "PIT: Invalid frequency 0");
        return;
    }

    LibC::uint32_t divisor = static_cast<LibC::uint32_t>(PIT_FREQUENCY / frequency);

    if (divisor == 0) {
        divisor = 1;
    }

    if (divisor > 0xFFFF) {
        Log(LogLevel::ERROR, "PIT: Divisor out of range");
        return;
    }

    send_command(PIT_COMMAND);
    set_divisor(divisor);

    Logf(LogLevel::INFO, "PIT: Initialized with frequency %u Hz, divisor %u", frequency, divisor);
}

void Pit::send_command(LibC::uint8_t command) noexcept
{
    Io::outb(PIT_COMMAND_PORT, command);
}

void Pit::set_divisor(LibC::uint16_t divisor) noexcept
{
    Io::outb(PIT_CHANNEL0_PORT, static_cast<LibC::uint8_t>(divisor & 0xFF));
    Io::outb(PIT_CHANNEL0_PORT, static_cast<LibC::uint8_t>((divisor >> 8) & 0xFF));
}
