#pragma once

#include <LibFK/Types/types.h>
#include <LibFK/Text/string.h>
#include <LibFK/Text/string_builder.h>

namespace fkernel {

struct MACAddress {
    uint8_t address[6];

    MACAddress() {
        for (int i = 0; i < 6; i++) address[i] = 0;
    }

    MACAddress(uint8_t a, uint8_t b, uint8_t c, uint8_t d, uint8_t e, uint8_t f) {
        address[0] = a; address[1] = b; address[2] = c;
        address[3] = d; address[4] = e; address[5] = f;
    }

    fk::text::String to_string() const {
        fk::text::StringBuilder sb;
        for (int i = 0; i < 6; i++) {
            sb.append_decimal(address[i]);
            if (i < 5) sb.append(":");
        }
        return sb.to_string();
    }

    bool operator==(const MACAddress& other) const {
        for (int i = 0; i < 6; i++) {
            if (address[i] != other.address[i]) return false;
        }
        return true;
    }
};

} // namespace fkernel
