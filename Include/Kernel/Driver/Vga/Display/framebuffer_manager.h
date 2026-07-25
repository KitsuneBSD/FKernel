#pragma once

#include <LibFK/Types/types.h>

class FrameBufferManager {
private:
    uint8_t* m_framebuffer = nullptr;
    uint8_t* m_back_buffer = nullptr;
    uint8_t* m_saved_buffer = nullptr;
    
    uint32_t m_fb_width = 0;
    uint32_t m_fb_height = 0;
    uint32_t m_fb_pitch = 0;
    uint16_t m_fb_bpp = 0;
    
    bool m_double_buffering_enabled = false;

public:
    FrameBufferManager() = default;
    
    // Initialize framebuffer from boot info
    void initialize_from_boot_info();
    
    // Memory management
    void allocate_back_buffer();
    void free_back_buffer();
    void swap_buffers();
    
    // Accessors
    uint8_t* get_framebuffer() const { return m_framebuffer; }
    uint8_t* get_back_buffer() const { return m_back_buffer; }
    uint8_t* get_render_buffer() const;
    
    uint32_t get_width() const { return m_fb_width; }
    uint32_t get_height() const { return m_fb_height; }
    uint32_t get_pitch() const { return m_fb_pitch; }
    uint16_t get_bpp() const { return m_fb_bpp; }
    
    bool is_double_buffering_enabled() const { return m_double_buffering_enabled; }
    
    // Screen save/restore
    void save_screen();
    void restore_screen();
    
    // VBlank synchronization
    void wait_vblank();
};