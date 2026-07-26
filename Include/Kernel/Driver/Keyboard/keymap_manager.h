#pragma once

#include <LibFK/Types/types.h>
#include <LibFK/Core/result.h>

namespace fkernel::drivers {

enum class KeyboardLayout {
    US,
    US_INTL,
    ABNT2
};

/**
 * @brief Manages keyboard layouts and character translation.
 * 
 * This class decouples the physical keyboard drivers from the 
 * logical mapping of keycodes to characters.
 */
class KeymapManager {
public:
    static KeymapManager& the();

    /**
     * @brief Translates a scancode/keycode to a character.
     * @param keycode The raw keycode from the driver.
     * @param shift Whether the Shift modifier is active.
     * @param alt Whether the Alt modifier is active.
     * @return The translated character, or 0 if no mapping exists.
     */
    char translate(uint8_t keycode, bool shift, bool alt, bool ctrl = false) const;

    /**
     * @brief Loads a keymap from a file in the VFS.
     * @param path Path to the keymap file.
     * @return Result of the operation.
     */
    fk::core::Result<void, fk::core::Error> load_from_file(const char* path);

    void set_layout(KeyboardLayout layout);
    KeyboardLayout layout() const { return m_current_layout; }

    void set_compose_mode(bool enabled) { m_compose_mode = enabled; }
    bool compose_mode() const { return m_compose_mode; }

private:
    KeymapManager();
    KeyboardLayout m_current_layout{KeyboardLayout::US_INTL};

    uint8_t m_map_normal[128]{0};
    uint8_t m_map_shift[128]{0};
    uint8_t m_map_alt[128]{0};

    bool m_compose_mode{true};  // dead keys active by default for US_INTL
    mutable char m_dead_key{0};         // buffered dead key character (0 = none)
    mutable char m_pending_flush{0};    // dead key literal to emit before next char

    void load_default_abnt2();
    void load_default_us();
    void load_default_us_intl();
    char resolve_dead_key(char dead, char next) const;
};

} // namespace fkernel::drivers
