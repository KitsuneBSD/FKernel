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

    auto* d = reinterpret_cast<uint8_t*>(dest);
    auto* s = reinterpret_cast<uint8_t const*>(src);

    while (n >= 8) {
        *reinterpret_cast<uint64_t*>(d) = *reinterpret_cast<uint64_t const*>(s);
        d += 8;
        s += 8;
        n -= 8;
    }
    while (n--) {
        *d++ = *s++;
    }
    return dest;
}

void* memset(void* dest, int ch, size_t n)
{
    if (!dest)
        return nullptr;

    uint8_t* d = (uint8_t*)dest;
    uint64_t pattern = 0;
    uint8_t c = static_cast<uint8_t>(ch);
    // Preenche pattern com c replicado em todos bytes
    for (int i = 0; i < 8; i++)
        pattern = (pattern << 8) | c;

    while (n >= 8) {
        *(uint64_t*)d = pattern;
        d += 8;
        n -= 8;
    }
    while (n--) {
        *d++ = c;
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

int strncmp(char const* s1, char const* s2, LibC::size_t n)
{
    for (size_t i = 0; i < n; ++i) {
        unsigned char c1 = (unsigned char)s1[i];
        unsigned char c2 = (unsigned char)s2[i];

        if (c1 != c2)
            return c1 - c2;

        if (c1 == '\0')
            return 0;
    }
    return 0;
}

char* strncpy(char* dest, char const* src, LibC::size_t n)
{
    size_t i = 0;

    while (i < n && src[i] != '\0') {
        dest[i] = src[i];
        ++i;
    }

    while (i < n) {
        dest[i] = '\0';
        ++i;
    }

    return dest;
}

LibC::size_t strlcpy(char* dst, char const* src, LibC::size_t size)
{
    if (!dst || !src)
        return 0;

    LibC::size_t i = 0;

    if (size > 0) {
        for (; i < size - 1 && src[i]; ++i)
            dst[i] = src[i];
        dst[i] = '\0';
    }

    while (src[i])
        ++i;

    return i;
}

int strcmp(char const* a, char const* b)
{
    while (*a && (*a == *b)) {
        ++a;
        ++b;
    }

    return *(unsigned char*)a - *(unsigned char*)b;
}

extern "C" char* strcpy(char* dest, char const* src)
{
    char* original = dest;
    while ((*dest++ = *src++))
        ;
    return original;
}

}
