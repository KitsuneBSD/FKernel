#include <LibFK/Terminal/ansi_parser.h>
#include <LibC/stdio.h>

namespace fk::terminal {

void AnsiParser::reset_sequence() {
    m_parameters.clear();
    m_current_parameter = 0;
    m_has_parameter = false;
    m_is_private = false;
    m_state = State::Normal;
}

void AnsiParser::process_data(const char* data, size_t size) {
    for (size_t i = 0; i < size; ++i) {
        process_char(data[i]);
    }
}

void AnsiParser::process_char(char c) {
    switch (m_state) {
        case State::Normal:
            if (c == '\033') {
                m_state = State::Escaped;
            } else if (c == 0x0E) { // SO (Shift Out) -> G1
                m_delegate.set_line_drawing_mode(true);
            } else if (c == 0x0F) { // SI (Shift In) -> G0
                m_delegate.set_line_drawing_mode(false);
            } else {
                m_delegate.put_char(c);
            }
            break;

        case State::Escaped:
            if (c == '[') {
                m_state = State::CSI;
            } else if (c == ']') {
                m_state = State::OSC;
            } else if (c == '(') {
                m_state = State::Escaped_Select_G0;
            } else if (c == '7') {
                m_delegate.save_cursor();
                m_state = State::Normal;
            } else if (c == '8') {
                m_delegate.restore_cursor();
                m_state = State::Normal;
            } else if (c == 'H') { // HTS - Set Tab Stop
                // TODO: Implement tab stops in delegate
                m_state = State::Normal;
            } else if (c == 'M') { // RI - Reverse Index (Scroll Up)
                m_delegate.scroll_down(1);
                m_state = State::Normal;
            } else {
                m_state = State::Normal;
            }
            break;

        case State::Escaped_Select_G0:
            if (c == '0') { // Special Graphics
                m_delegate.set_line_drawing_mode(true);
            } else if (c == 'B') { // US ASCII
                m_delegate.set_line_drawing_mode(false);
            }
            m_state = State::Normal;
            break;

        case State::CSI:
            if (c >= '0' && c <= '9') {
                m_current_parameter = m_current_parameter * 10 + (c - '0');
                m_has_parameter = true;
            } else if (c == ';') {
                m_parameters.push_back(m_current_parameter);
                m_current_parameter = 0;
                m_has_parameter = false;
            } else if (c == '?') {
                m_is_private = true;
            } else {
                if (m_has_parameter) {
                    m_parameters.push_back(m_current_parameter);
                }
                handle_csi(c);
                reset_sequence();
            }
            break;

        case State::OSC:
            if (c == '\a' || (c == '\\' && m_has_parameter)) { // OSC end: BEL or Esc \ (ST)
                m_state = State::Normal;
            } else if (c == '\033') {
                m_has_parameter = true; // Mark that we saw Esc
            }
            break;
    }
}

void AnsiParser::handle_csi(char c) {
    switch (c) {
        case 'm': { // SGR - Select Graphic Rendition
            uint8_t fg = 7;
            uint8_t bg = 0;
            static uint32_t fg_rgb = 0xAAAAAA;
            static uint32_t bg_rgb = 0x000000;
            static bool use_rgb = false;

            if (m_parameters.is_empty()) {
                m_delegate.set_colors(fg, bg);
                break;
            }
            
            for (size_t i = 0; i < m_parameters.size(); ++i) {
                uint16_t param = m_parameters[i];
                if (param == 0) { fg = 7; bg = 0; use_rgb = false; }
                else if (param >= 30 && param <= 37) { fg = param - 30; use_rgb = false; }
                else if (param >= 40 && param <= 47) { bg = param - 40; use_rgb = false; }
                else if (param >= 90 && param <= 97) { fg = (param - 90) + 8; use_rgb = false; }
                else if (param >= 100 && param <= 107) { bg = (param - 100) + 8; use_rgb = false; }
                else if (param == 38 || param == 48) {
                    bool is_fg = (param == 38);
                    if (i + 2 < m_parameters.size() && m_parameters[i+1] == 5) {
                        // 256 colors
                        uint8_t n = m_parameters[i+2];
                        // Simplistic mapping for 256 to 16 for now if no RGB
                        if (is_fg) fg = n % 16; else bg = n % 16;
                        i += 2;
                    } else if (i + 4 < m_parameters.size() && m_parameters[i+1] == 2) {
                        // TrueColor: 38;2;r;g;b
                        uint32_t rgb = (m_parameters[i+2] << 16) | (m_parameters[i+3] << 8) | m_parameters[i+4];
                        if (is_fg) fg_rgb = rgb; else bg_rgb = rgb;
                        use_rgb = true;
                        i += 4;
                    }
                }
            }
            if (use_rgb) m_delegate.set_colors_rgb(fg_rgb, bg_rgb);
            else m_delegate.set_colors(fg, bg);
            break;
        }
        case 'H':
        case 'f': { // CUP - Cursor Position
            uint16_t row = (m_parameters.size() > 0) ? m_parameters[0] : 1;
            uint16_t col = (m_parameters.size() > 1) ? m_parameters[1] : 1;
            m_delegate.move_cursor(row, col);
            break;
        }
        case 'A': m_delegate.move_cursor_up(m_parameters.size() > 0 ? m_parameters[0] : 1); break;
        case 'B': m_delegate.move_cursor_down(m_parameters.size() > 0 ? m_parameters[0] : 1); break;
        case 'C': m_delegate.move_cursor_forward(m_parameters.size() > 0 ? m_parameters[0] : 1); break;
        case 'D': m_delegate.move_cursor_back(m_parameters.size() > 0 ? m_parameters[0] : 1); break;
        case 'G': m_delegate.move_cursor(m_delegate.get_cursor_y() + 1, m_parameters.size() > 0 ? m_parameters[0] : 1); break; // CHA
        case 'd': m_delegate.move_cursor(m_parameters.size() > 0 ? m_parameters[0] : 1, m_delegate.get_cursor_x() + 1); break; // VPA
        
        case 'J': m_delegate.clear_screen(m_parameters.size() > 0 ? m_parameters[0] : 0); break;
        case 'K': m_delegate.clear_line(m_parameters.size() > 0 ? m_parameters[0] : 0); break;
        case 'X': m_delegate.erase_chars(m_parameters.size() > 0 ? m_parameters[0] : 1); break; // ECH
        
        case 'r': { // DECSTBM - Set Scrolling Region
            uint16_t top = (m_parameters.size() > 0) ? m_parameters[0] : 1;
            uint16_t bottom = (m_parameters.size() > 1) ? m_parameters[1] : 0;
            m_delegate.set_scroll_region(top, bottom);
            break;
        }
        case 'h': {
            if (m_is_private && m_parameters.size() > 0) {
                switch (m_parameters[0]) {
                    case 25: m_delegate.show_cursor(true); break;
                    case 47:
                    case 1047:
                    case 1049:
                        m_delegate.save_cursor();
                        m_delegate.save_screen();
                        m_delegate.clear_screen(2);
                        break;
                }
            }
            break;
        }
        case 'l': {
            if (m_is_private && m_parameters.size() > 0) {
                switch (m_parameters[0]) {
                    case 25: m_delegate.show_cursor(false); break;
                    case 47:
                    case 1047:
                    case 1049:
                        m_delegate.restore_screen();
                        m_delegate.restore_cursor();
                        break;
                }
            }
            break;
        }
        
        case '@': m_delegate.insert_chars(m_parameters.size() > 0 ? m_parameters[0] : 1); break; // ICH
        case 'P': m_delegate.delete_chars(m_parameters.size() > 0 ? m_parameters[0] : 1); break; // DCH
        case 'M': m_delegate.delete_lines(m_parameters.size() > 0 ? m_parameters[0] : 1); break; // DL
        case 'L': m_delegate.insert_lines(m_parameters.size() > 0 ? m_parameters[0] : 1); break; // IL
        case 'S': m_delegate.scroll_up(m_parameters.size() > 0 ? m_parameters[0] : 1); break;    // SU
        case 'T': m_delegate.scroll_down(m_parameters.size() > 0 ? m_parameters[0] : 1); break;  // SD
        
        case 'c': if (m_parameters.is_empty() || m_parameters[0] == 0) m_delegate.respond("\033[?1;2c"); break;
        case 'n': {
            if (m_parameters.is_empty() || m_parameters[0] == 5) m_delegate.respond("\033[0n");
            else if (m_parameters.size() > 0 && m_parameters[0] == 6) {
                char buf[32];
                snprintf(buf, sizeof(buf), "\033[%u;%uR", (uint32_t)m_delegate.get_cursor_y() + 1, (uint32_t)m_delegate.get_cursor_x() + 1);
                m_delegate.respond(buf);
            }
            break;
        }
        case 's': m_delegate.save_cursor(); break;
        case 'u': m_delegate.restore_cursor(); break;
        case 't': {
            if (m_parameters.size() > 0 && m_parameters[0] == 18) {
                char buf[64];
                snprintf(buf, sizeof(buf), "\033[8;%u;%ut", m_delegate.get_height(), m_delegate.get_width());
                m_delegate.respond(buf);
            }
            break;
        }
    }
}

} // namespace fk::terminal
