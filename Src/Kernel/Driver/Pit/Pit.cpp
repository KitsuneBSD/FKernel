#include <Kernel/Arch/x86_64/Cpu/Constants.h>
#include <Kernel/Arch/x86_64/Hardware/Io.h>
#include <Kernel/Arch/x86_64/Hardware/Io_Constants.h>
#include <Kernel/Driver/Pit/Pit.h>
#include <Kernel/Driver/Pit/Pit_constants.h>
#include <LibFK/enforce.h>
#include <LibFK/log.h>

Pit& Pit::Instance() noexcept
{
    static Pit instance;
    return instance;
}

void Pit::initialize(LibC::uint32_t frequency) noexcept
{
    if (FK::alert_if_f(frequency == 0, "PIT: Frequency must be greater than zero"))
        return;

    LibC::uint16_t divisor = compute_divisor(frequency);

    Logf(LogLevel::INFO, "PIT: Initializing — frequency=%llu Hz, divisor=%llu", frequency, divisor);

    // TODO: Use builder design pattern instead hard-coded initialize
    send_command(0b00110100);
    set_divisor(divisor);
}

void Pit::send_command(LibC::uint8_t command) noexcept
{
    Io::outb(PIT_COMMAND_PORT, command);
}

void Pit::set_divisor(LibC::uint16_t divisor) noexcept
{
    if (FK::alert_if_f(divisor < MIN_DIVISOR || divisor > MAX_DIVISOR,
            "PIT: Divisor out of range: %u", divisor))
        return;

    Io::outb(PIT_CHANNEL0_PORT, divisor & 0xFF);
    Io::outb(PIT_CHANNEL0_PORT, (divisor >> 8) & 0xFF);
}

LibC::uint16_t Pit::compute_divisor(LibC::uint32_t frequency) const noexcept
{
    return static_cast<LibC::uint16_t>(PIT_BASE_FREQUENCY / frequency);
}
