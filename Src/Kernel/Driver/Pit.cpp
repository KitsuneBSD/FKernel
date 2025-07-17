#include <Kernel/Arch/x86_64/Cpu/Constants.h>
#include <Kernel/Arch/x86_64/Hardware/Io.h>
#include <Kernel/Driver/Pit.h>
#include <LibFK/enforce.h>
#include <LibFK/log.h>

void Pit::initialize(LibC::uint32_t frequency) noexcept
{
    FK::enforcef(frequency > 0, "PIT: Frequency must be greater than 0");

    LibC::uint32_t divisor = (PIT_FREQUENCY + frequency / 2) / frequency;

    FK::alert_if_f(divisor == 0, "PIT: Computed divisor was zero, forcing to 1");
    if (divisor == 0)
        divisor = 1;

    FK::alert_if_f(frequency < 19 || frequency > PIT_FREQUENCY,
        "PIT: Frequency %u Hz is outside typical operating range (19Hz - %uHz)",
        frequency, PIT_FREQUENCY);

    FK::enforcef(divisor <= 0xFFFF, "PIT: Computed divisor (%u) exceeds 16-bit limit", divisor);

    send_command(PIT_COMMAND);
    set_divisor(static_cast<LibC::uint16_t>(divisor));

    Logf(LogLevel::INFO, "PIT: Initialized — frequency=%u Hz, divisor=%u", frequency, divisor);
}

void Pit::send_command(LibC::uint8_t command) noexcept
{
    Io::outb(PIT_COMMAND_PORT, command);
}

void Pit::set_divisor(LibC::uint16_t divisor) noexcept
{
    Io::outb(PIT_CHANNEL0_PORT, static_cast<LibC::uint8_t>(divisor & 0xFF));        // LSB
    Io::outb(PIT_CHANNEL0_PORT, static_cast<LibC::uint8_t>((divisor >> 8) & 0xFF)); // MSB
}
