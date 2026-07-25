#pragma once

#include <LibFK/Types/types.h>
#include <Kernel/Driver/Vga/vga_adapter.h>
#include <Kernel/Driver/Vga/display.h>

namespace fkernel {
namespace terminal {

/// @brief Handles visual representation and hardware interaction for terminals
class TerminalRenderer {
public:
    TerminalRenderer(uint16_t rows, uint16_t cols);
    ~TerminalRenderer() = default;

    void put_char(char c);
    void clear();
    void clear_screen(uint8_t mode, uint16_t rows);
    void clear_line(uint8_t mode, uint16_t rows, uint16_t cols);
    void move_cursor(uint16_t row, uint16_t col);
    
    void set_colors(uint8_t fg, uint8_t bg);
    void set_colors_rgb(uint32_t fg, uint32_t bg);
    
    void save_cursor();
    void restore_cursor();
    
    void scroll_up(uint16_t count, uint16_t rows);
    void scroll_down(uint16_t count, uint16_t rows);
    
    void insert_chars(uint16_t count, uint16_t rows, uint16_t cols);
    void delete_chars(uint16_t count, uint16_t rows, uint16_t cols);
    void erase_chars(uint16_t count, uint16_t rows, uint16_t cols);
    void insert_lines(uint16_t count, uint16_t rows);
    void delete_lines(uint16_t count, uint16_t rows);

    uint32_t cursor_x() const;
    uint32_t cursor_y() const;

private:
    [[maybe_unused]] uint16_t m_rows;
    [[maybe_unused]] uint16_t m_cols;
    uint16_t m_saved_x{0};
    uint16_t m_saved_y{0};
};

} // namespace terminal
} // namespace fkernel
