#include <Kernel/Driver/Vga/extended_font.h>
#include <LibFK/Utilities/memory.h>

namespace VgaExtended {

Glyph ExtendedFont::extended_glyphs[256];

ExtendedFont::ExtendedFont() {
    initialize_ascii_glyphs();
    initialize_extended_glyphs();
}

void ExtendedFont::initialize_ascii_glyphs() {
    // ASCII glyphs would be initialized here from a table
    // For now, they point to nullptr or are empty
    for (int i = 0; i < 95; ++i) {
        ascii_glyphs[i] = nullptr;
    }
}

void ExtendedFont::initialize_extended_glyphs() {
    // Extended glyphs initialization
    fk::memory::set(extended_glyphs, 0, sizeof(extended_glyphs));
}

const Glyph* ExtendedFont::get_ascii_glyph(uint8_t c) const {
    if (c < first_ascii_char || c > last_ascii_char) return nullptr;
    return ascii_glyphs[c - first_ascii_char];
}

const Glyph* ExtendedFont::get_utf8_glyph(const uint8_t* data, uint8_t& bytes_used) const {
    if (!data) return nullptr;
    
    if ((data[0] & 0x80) == 0) { // Standard ASCII
        bytes_used = 1;
        return get_ascii_glyph(data[0]);
    }
    
    // Simple UTF-8 Level 1 parsing (2 bytes sequences)
    if ((data[0] & 0xE0) == 0xC0) {
        bytes_used = 2;
        // Map to extended_glyphs based on sequence (simplified)
        uint16_t codepoint = ((data[0] & 0x1F) << 6) | (data[1] & 0x3F);
        if (codepoint < 256) return &extended_glyphs[codepoint];
    }
    
    return nullptr;
}

bool ExtendedFont::is_printable(uint8_t c) {
    return c >= 32 && c <= 126;
}

} // namespace VgaExtended
