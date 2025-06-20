#pragma once

#include <Kernel/Arch/x86_64/io.h>
#include <LibC/stddef.h>
#include <LibC/stdint.h>

namespace vga {

constexpr LibC::uintptr_t VGA_MEMORY = 0xB8000;
constexpr LibC::size_t VGA_WIDTH = 80;
constexpr LibC::size_t VGA_HEIGHT = 25;

enum class Color : LibC::uint8_t {
    Black = 0,
    Blue = 1,
    Green = 2,
    Cyan = 3,
    Red = 4,
    Magenta = 5,
    Brown = 6,
    LightGray = 7,
    DarkGray = 8,
    LightBlue = 9,
    LightGreen = 10,
    LightCyan = 11,
    LightRed = 12,
    Pink = 13,
    Yellow = 14,
    White = 15,
};

class Console {
public:
    void clear();
    void set_color(Color fg, Color bg);
    void putchar(char c);
    void write(char const* str);
    void write_hex(LibC::uint64_t value, bool prefix = true, bool uppercase = true);
    void write_dec(LibC::uint64_t value);

private:
    LibC::uint16_t* buffer = reinterpret_cast<LibC::uint16_t*>(VGA_MEMORY);
    LibC::size_t row = 0;
    LibC::size_t column = 0;
    LibC::uint8_t color = encode_color(Color::LightGray, Color::Black);

    static constexpr LibC::uint8_t encode_color(Color fg, Color bg)
    {
        return (static_cast<LibC::uint8_t>(bg) << 4) | static_cast<LibC::uint8_t>(fg);
    }

    LibC::uint16_t make_entry(char c) const;
    void putchar_raw(char c, LibC::size_t col, LibC::size_t row);
    void new_line();
    void scroll();
    void update_cursor();
};
} // namespace vga

inline vga::Console console;
