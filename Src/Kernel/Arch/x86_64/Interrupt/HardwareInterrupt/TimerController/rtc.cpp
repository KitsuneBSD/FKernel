#include <Kernel/Arch/x86_64/Interrupt/HardwareInterrupts/TimerController/rtc.h>
#include <Kernel/Arch/x86_64/io.h>

// RTC Registers
constexpr uint8_t RTC_REG_A = 0x0A;
constexpr uint8_t RTC_REG_B = 0x0B;

uint8_t RTCTimer::read_register(uint8_t reg) {
    outb(RTC_ADDRESS_PORT, reg);
    return inb(RTC_DATA_PORT);
}

void RTCTimer::write_register(uint8_t reg, uint8_t value) {
    outb(RTC_ADDRESS_PORT, reg);
    outb(RTC_DATA_PORT, value);
}

void RTCTimer::initialize(uint32_t frequency) {
    m_frequency = frequency;

    // Disable interrupts
    asm volatile("cli");

    // Read register B
    uint8_t reg_b = read_register(RTC_REG_B);

    // Set bit 6 to enable periodic interrupts
    write_register(RTC_REG_B, reg_b | 0x40);

    // Set frequency
    set_frequency(frequency);

    // Enable interrupts
    asm volatile("sti");
}

void RTCTimer::set_frequency(uint32_t frequency) {
    if (frequency == 0) {
        return;
    }

    // The RTC timer has a limited set of frequencies.
    // We need to find the closest rate.
    // The formula is frequency = 32768 >> (rate - 1)
    // So, rate = log2(32768 / frequency) + 1
    // Since we don't have log2, we can iterate to find the best rate.
    uint8_t rate = 15; // min frequency (2 Hz)
    if (frequency >= 1024) {
        rate = 6; // 1024 Hz
    } else if (frequency >= 512) {
        rate = 7; // 512 Hz
    } else if (frequency >= 256) {
        rate = 8; // 256 Hz
    } else if (frequency >= 128) {
        rate = 9; // 128 Hz
    } else if (frequency >= 64) {
        rate = 10; // 64 Hz
    } else if (frequency >= 32) {
        rate = 11; // 32 Hz
    } else if (frequency >= 16) {
        rate = 12; // 16 Hz
    } else if (frequency >= 8) {
        rate = 13; // 8 Hz
    } else if (frequency >= 4) {
        rate = 14; // 4 Hz
    }

    m_frequency = 32768 >> (rate - 1);

    // Disable interrupts
    asm volatile("cli");

    // Set the rate in the lower 4 bits of Register A
    uint8_t reg_a = read_register(RTC_REG_A);
    write_register(RTC_REG_A, (reg_a & 0xF0) | rate);

    // Enable interrupts
    asm volatile("sti");
}

void RTCTimer::sleep(uint64_t ms) {
    uint64_t start_ticks = get_ticks();
    uint64_t ticks_to_wait = ms * m_frequency / 1000;
    if (ticks_to_wait == 0) {
        ticks_to_wait = 1;
    }
    while (get_ticks() - start_ticks < ticks_to_wait) {
        asm volatile("hlt");
    }
}
