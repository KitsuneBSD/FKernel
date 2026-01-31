#pragma once

#include <LibFK/Types/types.h>
#include <LibFK/Core/Result.h>

namespace fkernel::drivers {

enum class KeyboardLayout {
    US,
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
    char translate(uint8_t keycode, bool shift, bool alt) const;

    /**
     * @brief Loads a keymap from a file in the VFS.
     * @param path Path to the keymap file.
     * @return Result of the operation.
     */
    fk::core::Result<void, fk::core::Error> load_from_file(const char* path);

    void set_layout(KeyboardLayout layout) { m_current_layout = layout; }
    KeyboardLayout layout() const { return m_current_layout; }

private:
    KeymapManager();
    KeyboardLayout m_current_layout{KeyboardLayout::ABNT2};

    uint8_t m_map_normal[128]{0};
    uint8_t m_map_shift[128]{0};
    uint8_t m_map_alt[128]{0};

    void load_default_abnt2();
};

} // namespace fkernel::drivers
