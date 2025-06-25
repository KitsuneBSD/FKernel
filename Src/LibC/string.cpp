#include <LibC/string.h>

namespace LibC {

static void reverse(char* str, int length)
{
    int start = 0;
    int end = length - 1;
    while (start < end) {
        char temp = str[start];
        str[start] = str[end];
        str[end] = temp;
        ++start;
        --end;
    }
}

LibC::size_t strlen(char const* str)
{
    char const* s = str;
    while (*s)
        ++s;
    return static_cast<LibC::size_t>(s - str);
}

char* itoa(int num, char* str, int base)
{
    if (base < 2 || base > 16) {
        str[0] = '\0';
        return str;
    }

    unsigned int value;
    bool is_negative = false;

    if (num == 0) {
        str[0] = '0';
        str[1] = '\0';
        return str;
    }

    if (num < 0 && base == 10) {
        is_negative = true;
        value = static_cast<unsigned int>(-(static_cast<long long>(num)));
    } else {
        value = static_cast<unsigned int>(num);
    }

    int i = 0;
    static constexpr char digits[] = "0123456789abcdef";

    while (value != 0) {
        unsigned int rem = value % static_cast<unsigned int>(base);
        str[i++] = digits[rem];
        value /= static_cast<unsigned int>(base);
    }

    if (is_negative) {
        str[i++] = '-';
    }

    str[i] = '\0';
    reverse(str, i);

    return str;
}

int atoi(char const* str)
{
    if (!str)
        return 0;

    int result = 0;
    int sign = 1;
    int i = 0;

    while (str[i] == ' ' || str[i] == '\t')
        ++i;

    if (str[i] == '-') {
        sign = -1;
        ++i;
    } else if (str[i] == '+') {
        ++i;
    }

    for (; str[i] >= '0' && str[i] <= '9'; ++i) {
        result = result * 10 + (str[i] - '0');
    }

    return sign * result;
}

extern "C" void* memcpy(void* dest, void const* src, LibC::size_t n)
{
    if (!dest || !src)
        return dest;

    char* d = static_cast<char*>(dest);
    char const* s = static_cast<char const*>(src);

    for (LibC::size_t i = 0; i < n; ++i) {
        d[i] = s[i];
    }

    return dest;
}

extern "C" void* memset(void* dest, int ch, LibC::size_t n)
{
    if (!dest) {
        return NULL;
    }

    char* d = static_cast<char*>(dest);

    for (LibC::size_t i = 0; i < n; ++i) {
        d[i] = static_cast<char>(ch);
    }

    return dest;
}

LibC::size_t utoa(LibC::uint64_t value, char* buffer, int base, bool uppercase)
{
    if (base < 2 || base > 16) {
        buffer[0] = '\0';
        return 0;
    }
    static char const* digits = uppercase ? "0123456789ABCDEF" : "0123456789abcdef";
    char temp[65];
    LibC::size_t i = 0;

    if (value == 0) {
        buffer[0] = '0';
        buffer[1] = '\0';
        return 1;
    }

    while (value != 0 && i < sizeof(temp) - 1) {
        temp[i++] = digits[value % static_cast<LibC::uint64_t>(base)];
        value /= static_cast<LibC::uint64_t>(base);
    }

    for (LibC::size_t j = 0; j < i; ++j) {
        buffer[j] = temp[i - j - 1];
    }
    buffer[i] = '\0';

    return i;
}

}
