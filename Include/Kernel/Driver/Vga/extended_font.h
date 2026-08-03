#pragma once

#include <Kernel/Driver/Vga/glyph.h>
#include <LibFK/Types/types.h>

namespace VgaExtended {

/// @brief Extended font supporting ASCII (32-126) + UTF-8 Level 1
class ExtendedFont {
public:
    ExtendedFont();

    /// @brief Get glyph for ASCII character
    const Glyph* get_ascii_glyph(uint8_t c) const;

    /// @brief Get glyph for UTF-8 sequence (returns nullptr if not supported)
    const Glyph* get_utf8_glyph(const uint8_t* data, uint8_t& bytes_used) const;

    /// @brief Check if character is printable
    static bool is_printable(uint8_t c);

    // Font properties
    uint8_t scale = 1;
    const Glyph* ascii_glyphs[95]; // ASCII 32-126
    static constexpr uint8_t first_ascii_char = 32;
    static constexpr uint8_t last_ascii_char = 126;

    // UTF-8 Level 1 common characters (partial implementation)
    static constexpr uint8_t UTF8_CEDILLA = 0xC2;
    static constexpr uint8_t UTF8_U_DIAERESIS_UML = 0xC3; // ü
    static constexpr uint8_t UTF8_A_GRAVE = 0xC3; // à
    static constexpr uint8_t UTF8_A_ACUTE = 0xC3; // á
    static constexpr uint8_t UTF8_A_CIRCUMFLEX = 0xC3; // â
    static constexpr uint8_t UTF8_A_TILDE = 0xC3; // ã
    static constexpr uint8_t UTF8_E_GRAVE = 0xC3; // è
    static constexpr uint8_t UTF8_E_ACUTE = 0xC3; // é
    static constexpr uint8_t UTF8_E_DIAERESIS = 0xC3; // ë
    static constexpr uint8_t UTF8_I_ACUTE = 0xC3; // í
    static constexpr uint8_t UTF8_I_CIRCUMFLEX = 0xC3; // î
    static constexpr uint8_t UTF8_N_TILDE = 0xC3; // ñ
    static constexpr uint8_t UTF8_O_GRAVE = 0xC3; // ò
    static constexpr uint8_t UTF8_O_DIAERESIS = 0xC3; // ö
    static constexpr uint8_t UTF8_O_TILDE = 0xC3; // õ
    static constexpr uint8_t UTF8_U_GRAVE = 0xC3; // ù
    static constexpr uint8_t UTF8_U_ACUTE = 0xC3; // ú

private:
    void initialize_ascii_glyphs();
    void initialize_extended_glyphs();
    bool utf8_starts_sequence(uint8_t c) const;

    // Extended glyph storage (256 entries for UTF-8 support)
    static Glyph extended_glyphs[256];
};

} // namespace VgaExtended
