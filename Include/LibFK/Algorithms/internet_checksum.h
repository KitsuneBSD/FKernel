#pragma once

#include <LibFK/Types/types.h>

namespace fk {
namespace algorithms {

// RFC 1071 one's-complement internet checksum over arbitrary byte ranges.
// Pass multiple regions by calling accumulate() repeatedly, then finalize().
class InternetChecksum {
    uint32_t m_sum{0};
public:
    void accumulate(const void* data, size_t len) {
        const uint16_t* p = reinterpret_cast<const uint16_t*>(data);
        while (len >= 2) {
            m_sum += *p++;
            len -= 2;
        }
        if (len)
            m_sum += *reinterpret_cast<const uint8_t*>(p);
    }

    uint16_t finalize() const {
        uint32_t s = m_sum;
        while (s >> 16) s = (s & 0xFFFF) + (s >> 16);
        return (uint16_t)(~s);
    }

    // Convenience: checksum over a single buffer
    static uint16_t compute(const void* data, size_t len) {
        InternetChecksum c;
        c.accumulate(data, len);
        return c.finalize();
    }
};

} // namespace algorithms
} // namespace fk
