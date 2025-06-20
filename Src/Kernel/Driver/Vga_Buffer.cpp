#include <Kernel/Driver/VgaBuffer.h>

LibC::uint16_t vga::Console::make_entry(char c) const
{
    return (static_cast<LibC::uint16_t>(color) << 8) | c;
}

void vga::Console::putchar_raw(char c, LibC::size_t col, LibC::size_t row)
{
    LibC::size_t const index = row * VGA_WIDTH + col;
    buffer[index] = make_entry(c);
}

void vga::Console::new_line()
{
    column = 0;
    if (++row == VGA_HEIGHT) {
        scroll();
        row = VGA_HEIGHT - 1;
    }
}

// TODO: Improve Scroll using memcpy (with SIMD/SSE) to optimize line copy
void vga::Console::scroll()
{
    for (LibC::size_t y = 1; y < VGA_HEIGHT; ++y) {
        for (LibC::size_t x = 0; x < VGA_WIDTH; ++x) {
            buffer[(y - 1) * VGA_WIDTH + x] = buffer[y * VGA_WIDTH + x];
        }
    }
    for (LibC::size_t x = 0; x < VGA_WIDTH; ++x) {
        buffer[(VGA_HEIGHT - 1) * VGA_WIDTH + x] = make_entry(' ');
    }
}

void vga::Console::update_cursor()
{
    LibC::uint16_t pos = row * VGA_WIDTH + column;
    Io::outb(0x3D4, 0x0F);
    Io::outb(0x3D5, pos & 0xFF);
    Io::outb(0x3D4, 0x0E);
    Io::outb(0x3D5, (pos >> 8) & 0xFF);
}

void vga::Console::clear()
{
    for (LibC::size_t y = 0; y < VGA_HEIGHT; ++y) {
        for (LibC::size_t x = 0; x < VGA_WIDTH; ++x) {
            putchar_raw(' ', x, y);
        }
    }
    row = column = 0;
    update_cursor();
}

void vga::Console::set_color(Color fg, Color bg)
{
    color = encode_color(fg, bg);
}

void vga::Console::putchar(char c)
{
    if (c == '\n') {
        new_line();
        update_cursor();
        return;
    }

    putchar_raw(c, column, row);
    if (++column == VGA_WIDTH) {
        new_line();
    }

    update_cursor();
}

void vga::Console::write_hex(LibC::uint64_t value, bool prefix, bool uppercase)
{
    if (prefix) {
        write("0x");
    }

    if (value == 0) {
        write("0");
        new_line();
        return;
    }

    char buffer[17];
    buffer[16] = '\0';
    int pos = 15;

    while (value != 0 && pos >= 0) {
        LibC::uint8_t digit = value & 0xF;
        buffer[pos--] = static_cast<char>((digit < 10)
                ? ('0' + digit)
                : ((uppercase ? 'A' : 'a') + (digit - 10)));
        value >>= 4;
    }

    write(&buffer[pos + 1]);
}

void vga::Console::write_dec(LibC::uint64_t value)
{
    char buffer[20];
    int i = 19;
    buffer[i--] = '\0';

    if (value == 0) {
        write("0");
        return;
    }

    while (value > 0 && i >= 0) {
        buffer[i--] = static_cast<char>('0' + (value % 10));
        value /= 10;
    }

    write(&buffer[i + 1]);
}

void vga::Console::write(char const* str)
{
    while (*str)
        putchar(*str++);
}
