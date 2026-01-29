#pragma once

#include <LibFK/Types/types.h>

namespace fkernel::drivers::vesa {

/**
 * @brief VBE Controller Information Block
 */
struct VbeInfoBlock {
    char signature[4];             // "VESA"
    uint16_t version;              // VBE version
    uint32_t oem_string_ptr;       // Real-mode Segment:Offset pointer to OEM string
    uint32_t capabilities;         // Capabilities of graphics controller
    uint32_t video_mode_ptr;       // Real-mode Segment:Offset pointer to supported video modes
    uint16_t total_memory;         // Number of 64kb memory blocks
    uint16_t oem_software_rev;     // VBE implementation software revision
    uint32_t oem_vendor_name_ptr;  // Pointer to Vendor Name String
    uint32_t oem_product_name_ptr; // Pointer to Product Name String
    uint32_t oem_product_rev_ptr;  // Pointer to Product Revision String
    uint8_t reserved[222];         // Reserved for VBE implementation
    uint8_t oem_data[256];         // Data Area for OEM String
} __attribute__((packed));

/**
 * @brief VBE Mode Information Block
 */
struct VbeModeInfoBlock {
    uint16_t attributes;           // Mode attributes
    uint8_t window_a, window_b;    // Window attributes
    uint16_t granularity;          // Window granularity
    uint16_t window_size;
    uint16_t segment_a, segment_b;
    uint32_t win_func_ptr;         // Real-mode pointer to window function
    uint16_t pitch;                // Bytes per scanline
    uint16_t width, height;        // Resolution
    uint8_t w_char, y_char, planes, bpp, banks;
    uint8_t memory_model, bank_size, image_pages;
    uint8_t reserved0;
 
    uint8_t red_mask, red_position;
    uint8_t green_mask, green_position;
    uint8_t blue_mask, blue_position;
    uint8_t reserved_mask, reserved_position;
    uint8_t direct_color_attributes;
 
    uint32_t framebuffer;          // Physical address of linear framebuffer (LFB)
    uint32_t off_screen_mem_off;
    uint16_t off_screen_mem_size;
    uint8_t reserved1[206];
} __attribute__((packed));

} // namespace fkernel::drivers::vesa
