#pragma once

// Type-safe format system with {} placeholders.
// Usage: format_v(buf, size, "Value: {} cpu: {}", val, cpu_id);
// A compile-time error fires if the {} placeholder count != argument count.

#include <LibC/stdio.h>
#include <LibFK/Types/types.h>

namespace fk {
namespace algorithms {

// ---- per-type argument stringification ------------------------------------

inline int format_arg(char* buf, size_t size, int v)               { return snprintf(buf, size, "%d", v); }
inline int format_arg(char* buf, size_t size, unsigned int v)      { return snprintf(buf, size, "%u", v); }
inline int format_arg(char* buf, size_t size, long v)              { return snprintf(buf, size, "%ld", v); }
inline int format_arg(char* buf, size_t size, unsigned long v)     { return snprintf(buf, size, "%lu", v); }
inline int format_arg(char* buf, size_t size, long long v)         { return snprintf(buf, size, "%lld", v); }
inline int format_arg(char* buf, size_t size, unsigned long long v){ return snprintf(buf, size, "%llu", v); }
inline int format_arg(char* buf, size_t size, bool v)              { return snprintf(buf, size, "%s", v ? "true" : "false"); }
inline int format_arg(char* buf, size_t size, char v)              { return snprintf(buf, size, "%c", (unsigned char)v); }
inline int format_arg(char* buf, size_t size, const char* v)       { return snprintf(buf, size, "%s", v ? v : "(null)"); }
inline int format_arg(char* buf, size_t size, void* v)             { return snprintf(buf, size, "%p", v); }
inline int format_arg(char* buf, size_t size, const void* v)       { return snprintf(buf, size, "%p", v); }

template <typename T>
inline int format_arg(char* buf, size_t size, T v) {
    return snprintf(buf, size, "%llu", static_cast<unsigned long long>(v));
}

// ---- compile-time placeholder counter ------------------------------------

consteval size_t count_placeholders(const char* s) {
    size_t n = 0;
    while (*s) {
        if (s[0] == '{' && s[1] == '}') { ++n; ++s; }
        ++s;
    }
    return n;
}

// ---- CheckedFormatString --------------------------------------------------
// Template parameters are the argument types — lets us verify count in consteval ctor.

template <typename... Args>
struct CheckedFormatString {
    const char* value;

    template <size_t N>
    consteval CheckedFormatString(const char (&s)[N]) : value(s) {
        if (count_placeholders(s) != sizeof...(Args))
            throw "format_v: number of {} placeholders must equal number of arguments";
    }
};

// ---- runtime format implementation ----------------------------------------

namespace detail {

inline int format_impl(char* buf, size_t size, const char* fmt) {
    size_t pos = 0;
    while (*fmt && pos + 1 < size) buf[pos++] = *fmt++;
    if (pos < size) buf[pos] = '\0';
    return (int)pos;
}

template <typename First, typename... Rest>
inline int format_impl(char* buf, size_t size, const char* fmt, const First& first, const Rest&... rest) {
    size_t pos = 0;
    while (*fmt && pos + 1 < size) {
        if (fmt[0] == '{' && fmt[1] == '}') {
            fmt += 2;
            char arg_buf[64];
            int n = format_arg(arg_buf, sizeof(arg_buf), first);
            for (int i = 0; i < n && pos + 1 < size; ++i) buf[pos++] = arg_buf[i];
            int rest_n = format_impl(buf + pos, size - pos, fmt, rest...);
            return (int)pos + rest_n;
        }
        buf[pos++] = *fmt++;
    }
    if (pos < size) buf[pos] = '\0';
    return (int)pos;
}

} // namespace detail

// ---- public API -----------------------------------------------------------

// type_identity puts the format-string parameter in a non-deduced context so
// Args... is deduced only from the explicit arguments, then the string literal
// is implicitly converted to CheckedFormatString<Args...> via the consteval ctor.
template <typename T>
struct type_identity { using type = T; };

template <typename... Args>
inline int format_v(char* buf, size_t size,
                    typename type_identity<CheckedFormatString<Args...>>::type fmt,
                    const Args&... args) {
    return detail::format_impl(buf, size, fmt.value, args...);
}

} // namespace algorithms
} // namespace fk
