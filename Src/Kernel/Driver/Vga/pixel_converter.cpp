#include <Kernel/Driver/Vga/pixel_converter.h>

namespace fkernel {

RawColor PixelConverter::get_raw_color(Color c) const {
    switch (c) {
        case Color::Black: return {0x00, 0x00, 0x00};
        case Color::Blue: return {0x00, 0x00, 0xAA};
        case Color::Green: return {0x00, 0xAA, 0x00};
        case Color::Cyan: return {0x00, 0xAA, 0xAA};
        case Color::Red: return {0xAA, 0x00, 0x00};
        case Color::Magenta: return {0xAA, 0x00, 0xAA};
        case Color::Brown: return {0xAA, 0x55, 0x00};
        case Color::LightGray: return {0xAA, 0xAA, 0xAA};
        case Color::DarkGray: return {0x55, 0x55, 0x55};
        case Color::LightBlue: return {0x55, 0x55, 0xFF};
        case Color::LightGreen: return {0x55, 0xFF, 0x55};
        case Color::LightCyan: return {0x55, 0xFF, 0xFF};
        case Color::LightRed: return {0xFF, 0x55, 0x55};
        case Color::LightMagenta: return {0xFF, 0x55, 0xFF};
        case Color::Yellow: return {0xFF, 0xFF, 0x55};
        case Color::White: return {0xFF, 0xFF, 0xFF};
        default: return {0, 0, 0};
    }
}

uint32_t PixelConverter::pack_pixel(uint8_t r, uint8_t g, uint8_t b) const {
    if (!boot::BootInfo::the().has_framebuffer()) return (r << 16) | (g << 8) | b;
    
    auto fb = boot::BootInfo::the().get_framebuffer_info();
    if (fb.red_mask == 0) return (r << 16) | (g << 8) | b;

    uint32_t pixel = 0;
    pixel |= ((r >> (8 - fb.red_mask)) << fb.red_pos);
    pixel |= ((g >> (8 - fb.green_mask)) << fb.green_pos);
    pixel |= ((b >> (8 - fb.blue_mask)) << fb.blue_pos);
    return pixel;
}

uint32_t PixelConverter::color_to_pixel(Color c) const {
    auto raw = get_raw_color(c);
    return pack_pixel(raw.r, raw.g, raw.b);
}

uint32_t PixelConverter::rgb_to_pixel(uint32_t rgb) const {
    return pack_pixel((rgb >> 16) & 0xFF, (rgb >> 8) & 0xFF, rgb & 0xFF);
}

} // namespace fkernel
