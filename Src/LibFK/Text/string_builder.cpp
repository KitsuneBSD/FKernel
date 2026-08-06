#include <LibFK/Text/string_builder.h>
#include <LibC/string.h>

namespace fk {
namespace text {

void StringBuilder::append(char c) {
    TRY_OR_FATAL(m_buffer.push_back(c));
}

void StringBuilder::append(const char* s) {
    if (!s) return;
    TRY_OR_FATAL(m_buffer.push_range(s, __builtin_strlen(s)));
}

void StringBuilder::append(const String& s) {
    append(s.c_str());
}

// Shared helper: write `n` in the given base (2/8/10/16) to a stack buffer and
// append it. `hex_digits` selects lowercase hex for base 16.
static void append_unsigned_impl(uint64_t n, int base, bool hex_upper,
                                  fk::containers::Vector<char>& out) {
    if (n == 0) { TRY_OR_FATAL(out.push_back('0')); return; }
    char buf[66]; // enough for base-2 64-bit + null
    int i = 65;
    buf[i] = '\0';
    while (n > 0) {
        unsigned digit = (unsigned)(n % (unsigned)base);
        if (digit < 10)
            buf[--i] = (char)('0' + digit);
        else
            buf[--i] = (char)((hex_upper ? 'A' : 'a') + digit - 10);
        n /= (uint64_t)base;
    }
    const char* p = &buf[i];
    while (*p) TRY_OR_FATAL(out.push_back(*p++));
}

void StringBuilder::append_decimal(int n) {
    if (n < 0) { append('-'); append_decimal(static_cast<uint64_t>(-n)); return; }
    append_decimal(static_cast<uint64_t>(n));
}

void StringBuilder::append_decimal(uint64_t n) {
    append_unsigned_impl(n, 10, false, m_buffer);
}

void StringBuilder::append_hex(uint64_t n, bool prefix) {
    if (prefix) { append('0'); append('x'); }
    append_unsigned_impl(n, 16, false, m_buffer);
}

void StringBuilder::append_octal(uint64_t n) {
    append_unsigned_impl(n, 8, false, m_buffer);
}

void StringBuilder::append_binary(uint64_t n) {
    append_unsigned_impl(n, 2, false, m_buffer);
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
    if (m_buffer.is_empty()) return String("");
    String result;
    result.reserve(m_buffer.size() + 1);
    for (char c : m_buffer) result.push_back(c);
    return result;
}

void StringBuilder::clear() {
    m_buffer.clear();
}

} // namespace text
} // namespace fk
