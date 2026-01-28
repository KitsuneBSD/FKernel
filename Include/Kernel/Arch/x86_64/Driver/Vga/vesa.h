#pragma once

#include <LibFK/Types/types.h>
#include <LibFK/Core/Result.h>
#include <LibFK/Core/Error.h>

namespace vesa {

struct VbeInfoBlock {
    char signature[4];             // "VESA"
    uint16_t version;              // VBE version; high byte is major, low byte is minor
    uint32_t oem_string_ptr;       // Segment:Offset pointer to OEM string
    uint32_t capabilities;         // Capabilities of graphics controller
    uint32_t video_mode_ptr;       // Segment:Offset pointer to supported video modes
    uint16_t total_memory;         // Number of 64kb memory blocks
    uint16_t oem_software_rev;     // VBE implementation software revision
    uint32_t oem_vendor_name_ptr;  // Pointer to Vendor Name String
    uint32_t oem_product_name_ptr; // Pointer to Product Name String
    uint32_t oem_product_rev_ptr;  // Pointer to Product Revision String
    uint8_t reserved[222];         // Reserved for VBE implementation
    uint8_t oem_data[256];         // Data Area for OEM String
} __attribute__((packed));

struct VbeModeInfoBlock {
    uint16_t attributes;           // deprecated, only bit 7 should be of interest
    uint8_t window_a, window_b;    // deprecated
    uint16_t granularity;          // deprecated
    uint16_t window_size;
    uint16_t segment_a, segment_b;
    uint32_t win_func_ptr;         // deprecated
    uint16_t pitch;                // number of bytes per horizontal line
    uint16_t width, height;        // width and height in pixels
    uint8_t w_char, y_char, planes, bpp, banks; // assorted
    uint8_t memory_model, bank_size, image_pages;
    uint8_t reserved0;
 
    uint8_t red_mask, red_position;
    uint8_t green_mask, green_position;
    uint8_t blue_mask, blue_position;
    uint8_t reserved_mask, reserved_position;
    uint8_t direct_color_attributes;
 
    uint32_t framebuffer;          // physical address of the linear frame buffer; write here to draw
    uint32_t off_screen_mem_off;
    uint16_t off_screen_mem_size;  // size of memory in the framebuffer but not being displayed
    uint8_t reserved1[206];
} __attribute__((packed));

class VESADriver {
public:
    static VESADriver& the();

    fk::core::Result<void, fk::core::Error> set_mode(uint16_t mode);
    fk::core::Result<void, fk::core::Error> set_resolution(uint32_t width, uint32_t height, uint32_t bpp);
    
    uint8_t* get_framebuffer() const { return reinterpret_cast<uint8_t*>(static_cast<uintptr_t>(m_mode_info.framebuffer)); }
    uint32_t get_width() const { return m_mode_info.width; }
    uint32_t get_height() const { return m_mode_info.height; }
    uint32_t get_pitch() const { return m_mode_info.pitch; }
    uint32_t get_bpp() const { return m_mode_info.bpp; }

    bool is_available() const { return m_vbe_supported; }

private:
    VESADriver();
    void find_vbe();

    [[maybe_unused]] VbeInfoBlock m_vbe_info{};
    VbeModeInfoBlock m_mode_info{};
    bool m_vbe_supported{false};
};

} // namespace vesa
