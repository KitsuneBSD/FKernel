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
            } else {
                m_delegate.put_char(c);
            }
            break;

        case State::Escaped:
            if (c == '[') {
                m_state = State::CSI;
            } else if (c == ']') {
                m_state = State::OSC;
            } else if (c == '7') {
                m_delegate.save_cursor();
                m_state = State::Normal;
            } else if (c == '8') {
                m_delegate.restore_cursor();
                m_state = State::Normal;
            } else {
                m_state = State::Normal;
            }
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
            if (c == '\a' || c == '\033') {
                m_state = State::Normal;
            }
            break;
    }
}

void AnsiParser::handle_csi(char c) {
    switch (c) {
        case 'm': { // SGR - Select Graphic Rendition
            uint8_t fg = 7;
            uint8_t bg = 0;
            if (m_parameters.is_empty()) {
                m_delegate.set_colors(fg, bg);
                break;
            }
            for (uint16_t param : m_parameters) {
                if (param == 0) { fg = 7; bg = 0; }
                else if (param >= 30 && param <= 37) { fg = param - 30; }
                else if (param >= 40 && param <= 47) { bg = param - 40; }
                else if (param >= 90 && param <= 97) { fg = (param - 90) + 8; }
                else if (param >= 100 && param <= 107) { bg = (param - 100) + 8; }
            }
            m_delegate.set_colors(fg, bg);
            break;
        }
        case 'H':
        case 'f': {
            uint16_t row = (m_parameters.size() > 0) ? m_parameters[0] : 1;
            uint16_t col = (m_parameters.size() > 1) ? m_parameters[1] : 1;
            m_delegate.move_cursor(row, col);
            break;
        }
        case 'A': m_delegate.move_cursor_up(m_parameters.size() > 0 ? m_parameters[0] : 1); break;
        case 'B': m_delegate.move_cursor_down(m_parameters.size() > 0 ? m_parameters[0] : 1); break;
        case 'C': m_delegate.move_cursor_forward(m_parameters.size() > 0 ? m_parameters[0] : 1); break;
        case 'D': m_delegate.move_cursor_back(m_parameters.size() > 0 ? m_parameters[0] : 1); break;
        case 'J': m_delegate.clear_screen(m_parameters.size() > 0 ? m_parameters[0] : 0); break;
        case 'K': m_delegate.clear_line(m_parameters.size() > 0 ? m_parameters[0] : 0); break;
        case 'r': {
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
                        m_delegate.restore_cursor();
                        break;
                }
            }
            break;
        }
        case 'I': m_delegate.insert_chars(m_parameters.size() > 0 ? m_parameters[0] : 1); break;
        case 'P': m_delegate.delete_chars(m_parameters.size() > 0 ? m_parameters[0] : 1); break;
        case 'M': m_delegate.delete_lines(m_parameters.size() > 0 ? m_parameters[0] : 1); break;
        case 'L': m_delegate.insert_lines(m_parameters.size() > 0 ? m_parameters[0] : 1); break;
        case 'S': m_delegate.scroll_up(m_parameters.size() > 0 ? m_parameters[0] : 1); break;
        case 'T': m_delegate.scroll_down(m_parameters.size() > 0 ? m_parameters[0] : 1); break;
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
    }
}

} // namespace fk::terminal