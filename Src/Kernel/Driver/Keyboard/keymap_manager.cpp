#include <Kernel/Driver/Keyboard/keymap_manager.h>
#include <Kernel/Fs/Vfs/virtual_filesystem.h>
#include <Kernel/Fs/Vfs/definitions.h>
#include <LibFK/Algorithms/log.h>
#include <LibFK/Core/result.h>
#include <LibFK/Utilities/memory.h>

namespace fkernel::drivers {

// Row helper: 16 entries
#define R16(a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p) a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p

// Sentinel: marks a dead key position in the keymap table
static constexpr uint8_t DEAD_KEY = 0x01;

KeymapManager::KeymapManager() {
    load_default_us_intl();
}

KeymapManager& KeymapManager::the() {
    static KeymapManager instance;
    return instance;
}

void KeymapManager::set_layout(KeyboardLayout layout) {
    m_current_layout = layout;
    m_dead_key = 0;
    if (layout == KeyboardLayout::ABNT2) {
        load_default_abnt2();
    } else if (layout == KeyboardLayout::US_INTL) {
        load_default_us_intl();
    } else {
        load_default_us();
    }
}

void KeymapManager::load_default_abnt2() {
    static const uint8_t abnt2_normal[128] = {
        R16(0, 27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b', '\t'),
        R16('q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', 0, '[', '\n', 0, 'a', 's'),
        R16('d', 'f', 'g', 'h', 'j', 'k', 'l', 'c', '~', '\'', 0, ']', 'z', 'x', 'c', 'v'),
        R16('b', 'n', 'm', ',', '.', ';', 0, '*', 0, ' ', 0, 0, 0, 0, 0, 0),
        R16(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0),
        R16(0, 0, 0, 0, 0, 0, '\\', 0, 0, 0, 0, 0, 0, 0, 0, 0),
        R16(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0),
        R16(0, 0, 0, '/', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0)
    };

    static const uint8_t abnt2_shift[128] = {
        R16(0, 27, '!', '@', '#', '$', '%', 0, '&', '*', '(', ')', '_', '+', '\b', '\t'),
        R16('Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', 0, '{', '\n', 0, 'A', 'S'),
        R16('D', 'F', 'G', 'H', 'J', 'K', 'L', 'C', '^', '"', 0, '}', 'Z', 'X', 'C', 'V'),
        R16('B', 'N', 'M', '<', '>', ':', 0, '*', 0, ' ', 0, 0, 0, 0, 0, 0),
        R16(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0),
        R16(0, 0, 0, 0, 0, 0, '|', 0, 0, 0, 0, 0, 0, 0, 0, 0),
        R16(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0),
        R16(0, 0, 0, '?', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0)
    };

    fk::memory::copy(m_map_normal, abnt2_normal, 128);
    fk::memory::copy(m_map_shift, abnt2_shift, 128);
    fk::memory::set(m_map_alt, 0, 128);
}

void KeymapManager::load_default_us() {
    static const uint8_t us_normal[128] = {
        R16(0, 27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b', '\t'),
        R16('q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n', 0, 'a', 's'),
        R16('d', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`', 0, '\\', 'z', 'x', 'c', 'v'),
        R16('b', 'n', 'm', ',', '.', '/', 0, '*', 0, ' ', 0, 0, 0, 0, 0, 0),
        R16(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0),
        R16(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0),
        R16(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0),
        R16(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0)
    };

    static const uint8_t us_shift[128] = {
        R16(0, 27, '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', '\b', '\t'),
        R16('Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', '\n', 0, 'A', 'S'),
        R16('D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"', '~', 0, '|', 'Z', 'X', 'C', 'V'),
        R16('B', 'N', 'M', '<', '>', '?', 0, '*', 0, ' ', 0, 0, 0, 0, 0, 0),
        R16(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0),
        R16(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0),
        R16(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0),
        R16(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0)
    };

    fk::memory::copy(m_map_normal, us_normal, 128);
    fk::memory::copy(m_map_shift, us_shift, 128);
    fk::memory::set(m_map_alt, 0, 128);
}

void KeymapManager::load_default_us_intl() {
    // US International: same as US except dead keys at scancodes:
    //   0x28 (apostrophe), 0x29 (backtick), shift+6 = ^, shift+backtick = ~
    // Dead keys produce 0x00 in the map when compose_mode is on;
    // translate() handles the combining logic.
    static const uint8_t us_intl_normal[128] = {
        R16(0, 27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b', '\t'),
        R16('q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n', 0, 'a', 's'),
        R16('d', 'f', 'g', 'h', 'j', 'k', 'l', ';', DEAD_KEY, DEAD_KEY, 0, '\\', 'z', 'x', 'c', 'v'),
        R16('b', 'n', 'm', ',', '.', '/', 0, '*', 0, ' ', 0, 0, 0, 0, 0, 0),
        R16(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0),
        R16(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0),
        R16(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0),
        R16(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0)
    };
    //   Row3[8] = 0x28 = apostrophe dead key (')
    //   Row3[9] = 0x29 = backtick dead key (`)
    // DEAD_KEY sentinel marks dead key positions for compose logic

    static const uint8_t us_intl_shift[128] = {
        R16(0, 27, '!', '@', '#', '$', '%', DEAD_KEY, '&', '*', '(', ')', '_', '+', '\b', '\t'),
        R16('Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', '\n', 0, 'A', 'S'),
        R16('D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"', DEAD_KEY, 0, '|', 'Z', 'X', 'C', 'V'),
        R16('B', 'N', 'M', '<', '>', '?', 0, '*', 0, ' ', 0, 0, 0, 0, 0, 0),
        R16(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0),
        R16(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0),
        R16(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0),
        R16(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0)
    };
    //   Row1[7] = Shift+6 = ^ dead key (circumflex)
    //   Row3[9] = Shift+` = ~ dead key (tilde)

    // AltGr map: right Alt + vowel → accented lowercase
    static const uint8_t us_intl_alt[128] = {
        R16(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0),
        R16(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0),
        R16(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0),
        R16(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0),
        R16(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0),
        R16(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0),
        R16(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0),
        R16(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0)
    };

    fk::memory::copy(m_map_normal, us_intl_normal, 128);
    fk::memory::copy(m_map_shift, us_intl_shift, 128);
    fk::memory::copy(m_map_alt, us_intl_alt, 128);
}

char KeymapManager::resolve_dead_key(char dead, char next) const {
    // Grave accent (`)
    if (dead == '`') {
        if (next == 'a') return '\xe0'; // à
        if (next == 'e') return '\xe8'; // è
        if (next == 'i') return '\xec'; // ì
        if (next == 'o') return '\xf2'; // ò
        if (next == 'u') return '\xf9'; // ù
        if (next == 'A') return '\xc0'; // À
        if (next == 'E') return '\xc8'; // È
        if (next == 'I') return '\xcc'; // Ì
        if (next == 'O') return '\xd2'; // Ò
        if (next == 'U') return '\xd9'; // Ù
        if (next == ' ') return '`';
        return 0; // no match: both chars emitted separately
    }
    // Acute accent (')
    if (dead == '\'') {
        if (next == 'a') return '\xe1'; // á
        if (next == 'e') return '\xe9'; // é
        if (next == 'i') return '\xed'; // í
        if (next == 'o') return '\xf3'; // ó
        if (next == 'u') return '\xfa'; // ú
        if (next == 'A') return '\xc1'; // Á
        if (next == 'E') return '\xc9'; // É
        if (next == 'I') return '\xcd'; // Í
        if (next == 'O') return '\xd3'; // Ó
        if (next == 'U') return '\xda'; // Ú
        if (next == 'c') return '\xe7'; // ç
        if (next == 'C') return '\xc7'; // Ç
        if (next == ' ') return '\'';
        return 0;
    }
    // Circumflex (^)
    if (dead == '^') {
        if (next == 'a') return '\xe2'; // â
        if (next == 'e') return '\xea'; // ê
        if (next == 'i') return '\xee'; // î
        if (next == 'o') return '\xf4'; // ô
        if (next == 'u') return '\xfb'; // û
        if (next == 'A') return '\xc2'; // Â
        if (next == 'E') return '\xca'; // Ê
        if (next == 'I') return '\xce'; // Î
        if (next == 'O') return '\xd4'; // Ô
        if (next == 'U') return '\xdb'; // Û
        if (next == ' ') return '^';
        return 0;
    }
    // Tilde (~)
    if (dead == '~') {
        if (next == 'a') return '\xe3'; // ã
        if (next == 'o') return '\xf5'; // õ
        if (next == 'n') return '\xf1'; // ñ
        if (next == 'A') return '\xc3'; // Ã
        if (next == 'O') return '\xd5'; // Õ
        if (next == 'N') return '\xd1'; // Ñ
        if (next == ' ') return '~';
        return 0;
    }
    return 0;
}

fk::core::Result<void, fk::core::Error> KeymapManager::load_from_file(const char* path) {
    auto file_or_error = VirtualFileSystem::the().open(path, 0); // O_RDONLY
    if (file_or_error.is_error()) {
        return file_or_error.error();
    }

    auto file = file_or_error.value();
    
    uint8_t header[8];
    TRY(file->seek(0, SeekMode::Set));
    TRY(file->read(8, header));

    if (fk::memory::compare(header, "FKMAP", 5) != 0) {
        fk::algorithms::klog("KEYMAP", "Invalid keymap header in %s", path);
        return fk::core::Error::InvalidParameter;
    }

    // Load tables: Normal, Shift, Alt (128 bytes each)
    TRY(file->seek(8, SeekMode::Set));
    TRY(file->read(128, m_map_normal));
    
    TRY(file->seek(136, SeekMode::Set));
    TRY(file->read(128, m_map_shift));
    
    TRY(file->seek(264, SeekMode::Set));
    TRY(file->read(128, m_map_alt));

    fk::algorithms::klog("KEYMAP", "Loaded keymap from %s", path);
    return {};
}

char KeymapManager::translate(uint8_t keycode, bool shift, bool alt, bool ctrl) const {
    if (keycode >= 128)
        return 0;

    // Ctrl modifier: produce control characters for letters and special keys
    if (ctrl) {
        // Ctrl+A through Ctrl+Z → \x01 through \x1A (scancodes 0x1E..0x39)
        if (keycode >= 0x1E && keycode <= 0x39) {
            return static_cast<char>(keycode - 0x1E + 1);
        }
        // Ctrl+[ → \x1B (ESC), Ctrl+\ → \x1C, Ctrl+] → \x1D
        if (keycode == 0x1A) return '\x1B';
        if (keycode == 0x2B) return '\x1C';
        if (keycode == 0x1B) return '\x1D';
        return 0;
    }

    // AltGr modifier: right Alt + key → accented character
    if (alt && m_map_alt[keycode] != 0) {
        return (char)m_map_alt[keycode];
    }
    // Special hardcoded combination: Alt + Q -> / (only if alt map doesn't have it)
    if (alt && keycode == 0x10 && m_map_alt[keycode] == 0) {
        return '/';
    }

    // Determine the raw character from the appropriate map
    uint8_t raw = shift ? m_map_shift[keycode] : m_map_normal[keycode];

    // Dead key handling (US_INTL compose mode)
    if (m_compose_mode && raw == DEAD_KEY) {
        // Identify which dead key this is based on scancode + shift
        char dead_char = 0;
        if (keycode == 0x28 && !shift) dead_char = '\'';   // apostrophe key
        else if (keycode == 0x29 && !shift) dead_char = '`'; // backtick key
        else if (keycode == 0x08 && shift) dead_char = '^';  // Shift+6
        else if (keycode == 0x29 && shift) dead_char = '~';  // Shift+`

        if (m_dead_key) {
            // Previous key was a dead key — try to combine
            char prev = m_dead_key;
            char combined = resolve_dead_key(prev, dead_char);
            m_dead_key = 0;
            if (combined) return combined;
            // No combination: emit previous dead key literal, buffer current as new dead key
            m_dead_key = dead_char;
            return prev;
        }
        // Buffer this dead key and wait for the next keypress
        m_dead_key = dead_char;
        return 0; // don't emit anything yet
    }

    // Flush pending dead key before processing this character
    if (m_pending_flush) {
        char c = m_pending_flush;
        m_pending_flush = 0;
        return c;
    }

    // If a dead key was buffered and this key arrived, try to combine
    if (m_dead_key) {
        char dead = m_dead_key;
        m_dead_key = 0;

        if (raw != DEAD_KEY) {
            // Try to combine dead key with current character
            char combined = resolve_dead_key(dead, static_cast<char>(raw));
            if (combined) return combined;
            // No combination: emit dead key literal, then current char
            m_pending_flush = static_cast<char>(raw);
            return dead;
        }
        // Current key is also a dead key: emit previous dead key literal, start new dead key
        m_dead_key = static_cast<char>(raw);
        return dead;
    }

    return (char)raw;
}

} // namespace fkernel::drivers