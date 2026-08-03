#pragma once

#include <Kernel/Driver/Vga/Types/color.h>
#include <Kernel/Driver/Vga/raw_color.h>
#include <Kernel/Boot/Core/boot_info.h>
#include <LibFK/Types/types.h>

namespace fkernel {

class PixelConverter {
public:
    PixelConverter() = default;

    uint32_t color_to_pixel(Color c) const;
    uint32_t rgb_to_pixel(uint32_t rgb) const;

private:
    RawColor get_raw_color(Color c) const;
    uint32_t pack_pixel(uint8_t r, uint8_t g, uint8_t b) const;
};

} // namespace fkernel
