#pragma once

#include <LibFK/Types/types.h>

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

    void set_layout(KeyboardLayout layout) { m_current_layout = layout; }
    KeyboardLayout layout() const { return m_current_layout; }

private:
    KeymapManager() = default;
    KeyboardLayout m_current_layout{KeyboardLayout::ABNT2};
};

} // namespace fkernel::drivers
