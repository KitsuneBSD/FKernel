#pragma once

#include <LibFK/Types/types.h>
#include <LibFK/Algorithms/Generic/string_algorithms.h>

namespace fk {
namespace algorithms {

// Format an 8.3 FAT directory entry name into a null-terminated C string.
// name_field: 8-byte space-padded name, ext_field: 3-byte space-padded extension.
static inline void format_83_name(const char* name_field, const char* ext_field, char* out) {
    int name_len = 8;
    while (name_len > 0 && name_field[name_len - 1] == ' ') --name_len;
    int ext_len = 3;
    while (ext_len > 0 && ext_field[ext_len - 1] == ' ') --ext_len;
    int pos = 0;
    for (int i = 0; i < name_len; ++i) out[pos++] = name_field[i];
    if (ext_len > 0) {
        out[pos++] = '.';
        for (int i = 0; i < ext_len; ++i) out[pos++] = ext_field[i];
    }
    out[pos] = '\0';
}

// Case-insensitive comparison of a formatted 8.3 name against a lookup name.
// Calls format_83_name internally then delegates to iequal.
static inline bool fat_name_eq_83(const char* name_field, const char* ext_field,
                                   const char* lookup) {
    char buf[13];
    format_83_name(name_field, ext_field, buf);
    return iequal(buf, lookup);
}

// Unpack one FAT LFN entry into lfn_buf[] at the correct char positions.
// raw: pointer to the 32-byte LFN directory entry.
// order: the LFN order field (1-based, 0x40 stripped by caller).
static inline void lfn_fill_chars(const uint8_t* raw, int order, char* lfn_buf) {
    int base = (order - 1) * 13;
    const uint16_t* n1 = reinterpret_cast<const uint16_t*>(raw + 1);
    const uint16_t* n2 = reinterpret_cast<const uint16_t*>(raw + 14);
    const uint16_t* n3 = reinterpret_cast<const uint16_t*>(raw + 28);
    auto put = [&](int pos, uint16_t c) {
        if (pos >= 255) return;
        lfn_buf[pos] = (c == 0x0000 || c == 0xFFFF) ? '\0' : static_cast<char>(c & 0x7F);
    };
    for (int i = 0; i < 5; ++i) put(base + i, n1[i]);
    for (int i = 0; i < 6; ++i) put(base + 5 + i, n2[i]);
    for (int i = 0; i < 2; ++i) put(base + 11 + i, n3[i]);
}

} // namespace algorithms
} // namespace fk
