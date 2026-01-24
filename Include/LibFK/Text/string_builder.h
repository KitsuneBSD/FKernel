#pragma once

#include <LibFK/Container/vector.h>
#include <LibFK/Text/string.h>
#include <LibFK/Types/types.h>

namespace fk {
namespace text {

class StringBuilder {
public:
    StringBuilder() = default;
    ~StringBuilder() = default;

    void append(char c);
    void append(const char* s);
    void append(const String& s);
    void append_decimal(int n);
    void append_decimal(uint64_t n);

    String to_string() const;
    void clear();

    size_t length() const { return m_buffer.size(); }
    const char* characters() const { return m_buffer.begin(); }

private:
    fk::containers::Vector<char> m_buffer;
};

} // namespace text
} // namespace fk
