#include <LibFK/Text/string_builder.h>
#include <LibC/string.h>

namespace fk {
namespace text {

void StringBuilder::append(char c) {
    m_buffer.push_back(c);
}

void StringBuilder::append(const char* s) {
    if (!s) return;
    while (*s) {
        m_buffer.push_back(*s++);
    }
}

void StringBuilder::append(const String& s) {
    append(s.c_str());
}

void StringBuilder::append_decimal(int n) {
    if (n < 0) {
        append('-');
        append_decimal(static_cast<uint64_t>(-n));
        return;
    }
    append_decimal(static_cast<uint64_t>(n));
}

void StringBuilder::append_decimal(uint64_t n) {
    if (n == 0) {
        append('0');
        return;
    }

    char buf[21];
    int i = 20;
    buf[i] = '\0';

    while (n > 0) {
        buf[--i] = (n % 10) + '0';
        n /= 10;
    }

    append(&buf[i]);
}

String StringBuilder::to_string() const {
    if (m_buffer.is_empty())
        return String("");

    // Create string directly from buffer data without intermediate buffer
    // This prevents use-after-free when temp_buffer goes out of scope
    String result;
    result.reserve(m_buffer.size() + 1);
    
    for (char c : m_buffer) {
        result.push_back(c);
    }
    
    return result;
}

void StringBuilder::clear() {
    m_buffer.clear();
}

} // namespace text
} // namespace fk
