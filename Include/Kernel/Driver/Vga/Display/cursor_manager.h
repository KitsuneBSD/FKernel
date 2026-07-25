#pragma once

#include <LibFK/Types/types.h>

class CursorManager {
private:
    uint32_t m_cursor_x = 0;
    uint32_t m_cursor_y = 0;
    bool m_cursor_visible = true;

public:
    CursorManager() = default;
    
    // Position management
    void set_position(uint32_t x, uint32_t y);
    uint32_t get_x() const { return m_cursor_x; }
    uint32_t get_y() const { return m_cursor_y; }
    
    // Visibility
    void show_cursor(bool visible);
    bool is_visible() const { return m_cursor_visible; }
    
    // Movement
    void advance_cursor();
    void newline();
    void tab();
    void backspace();
};