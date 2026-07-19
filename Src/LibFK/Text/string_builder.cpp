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

void StringBuilder::append_hex(uint64_t n, bool prefix) {
    if (prefix) {
        append('0');
        append('x');
    }
    if (n == 0) {
        append('0');
        return;
    }
    char buf[17];
    int i = 16;
    buf[i] = '\0';
    while (n > 0) {
        uint64_t digit = n & 0xF;
        buf[--i] = (digit < 10) ? ('0' + digit) : ('a' + digit - 10);
        n >>= 4;
    }
    append(&buf[i]);
}

void StringBuilder::append_octal(uint64_t n) {
    if (n == 0) {
        append('0');
        return;
    }
    char buf[23];
    int i = 22;
    buf[i] = '\0';
    while (n > 0) {
        buf[--i] = '0' + (n & 7);
        n >>= 3;
    }
    append(&buf[i]);
}

void StringBuilder::append_binary(uint64_t n) {
    if (n == 0) {
        append('0');
        return;
    }
    char buf[65];
    int i = 64;
    buf[i] = '\0';
    while (n > 0) {
        buf[--i] = '0' + (n & 1);
        n >>= 1;
    }
    append(&buf[i]);
}

void StringBuilder::append_float(double f, int precision) {
    if (__builtin_isnan(f)) { append("nan"); return; }
    if (__builtin_isinf(f)) { append(f < 0.0 ? "-inf" : "inf"); return; }
    if (f < 0.0) { append('-'); f = -f; }
    uint64_t integer_part = static_cast<uint64_t>(f);
    append_decimal(integer_part);
    if (precision <= 0) return;
    append('.');
    double frac = f - static_cast<double>(integer_part);
    for (int i = 0; i < precision; ++i) {
        frac *= 10.0;
        int digit = static_cast<int>(frac);
        append(static_cast<char>('0' + digit));
        frac -= static_cast<double>(digit);
    }
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
