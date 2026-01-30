#pragma once

#include <LibFK/Types/types.h>
#include <LibFK/Container/vector.h>

namespace fk::terminal {

/**
 * @brief Delegate interface for AnsiParser.
 * The terminal implementation must provide these methods.
 */
class AnsiDelegate {
public:
    virtual ~AnsiDelegate() = default;

    virtual void put_char(char c) = 0;
    virtual void move_cursor(uint16_t row, uint16_t col) = 0;
    virtual void move_cursor_up(uint16_t rows) = 0;
    virtual void move_cursor_down(uint16_t rows) = 0;
    virtual void move_cursor_forward(uint16_t cols) = 0;
    virtual void move_cursor_back(uint16_t cols) = 0;
    virtual void set_colors(uint8_t fg, uint8_t bg) = 0;
    virtual void clear_screen(uint8_t mode) = 0; // 0: end, 1: start, 2: all
    virtual void clear_line(uint8_t mode) = 0;   // 0: end, 1: start, 2: all
    virtual void set_scroll_region(uint16_t top, uint16_t bottom) = 0;
    virtual void save_cursor() = 0;
    virtual void restore_cursor() = 0;
    virtual void save_screen() = 0;
    virtual void restore_screen() = 0;
    virtual void show_cursor(bool visible) = 0;
    virtual uint32_t get_cursor_x() = 0;
    virtual uint32_t get_cursor_y() = 0;
    
    /// @brief Send a response string back to the terminal input
    virtual void respond(const char* data) = 0;
    
    // Additional methods for vi compatibility
    virtual void insert_chars(uint16_t count) = 0;
    virtual void delete_chars(uint16_t count) = 0;
    virtual void erase_chars(uint16_t count) = 0; // ECH
    virtual void insert_lines(uint16_t count) = 0;
    virtual void delete_lines(uint16_t count) = 0;
    virtual void scroll_up(uint16_t count) = 0;
    virtual void scroll_down(uint16_t count) = 0;

    // Line drawing and Charset
    virtual void set_line_drawing_mode(bool enabled) = 0;
};

/**
 * @brief State-machine based ANSI escape sequence parser (VT100/VT102/XTerm).
 */
class AnsiParser {
public:
    enum class State {
        Normal,
        Escaped,
        Escaped_Select_G0,
        CSI, // Control Sequence Introducer: Esc [
        OSC, // Operating System Command: Esc ]
    };

    explicit AnsiParser(AnsiDelegate& delegate) : m_delegate(delegate) {}

    void process_char(char c);
    void process_data(const char* data, size_t size);

private:
    void handle_csi(char c);
    void reset_sequence();

    AnsiDelegate& m_delegate;
    State m_state{State::Normal};
    
    fk::containers::Vector<uint16_t> m_parameters;
    uint16_t m_current_parameter{0};
    bool m_has_parameter{false};
    bool m_is_private{false}; // CSI ?
};

} // namespace fk::terminal
