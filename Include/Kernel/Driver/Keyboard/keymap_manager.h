#pragma once

#include <LibFK/Types/types.h>
#include <LibFK/Core/result.h>

namespace fkernel::drivers {

enum class KeyboardLayout {
    US,
    US_INTL,
    ABNT2
};

class KeymapManager {
  KeymapManager();
  KeymapManager(const KeymapManager&) = delete;
  KeymapManager& operator=(const KeymapManager&) = delete;
  KeymapManager(KeymapManager&&) = delete;
  KeymapManager& operator=(KeymapManager&&) = delete;

  bool m_is_initialized{false};
  KeyboardLayout m_current_layout{KeyboardLayout::US_INTL};

  uint8_t m_map_normal[128]{0};
  uint8_t m_map_shift[128]{0};
  uint8_t m_map_alt[128]{0};

  bool m_compose_mode{true};
  mutable char m_dead_key{0};
  mutable char m_pending_flush{0};

  void load_default_abnt2();
  void load_default_us();
  void load_default_us_intl();
  char resolve_dead_key(char dead, char next) const;

public:
  static KeymapManager& the();

  bool is_initialized() const { return m_is_initialized; }
  void initialize();

  char translate(uint8_t keycode, bool shift, bool alt, bool ctrl = false) const;

  fk::core::Result<void, fk::core::Error> load_from_file(const char* path);

  void set_layout(KeyboardLayout layout);
  KeyboardLayout layout() const { return m_current_layout; }

  void set_compose_mode(bool enabled) { m_compose_mode = enabled; }
  bool compose_mode() const { return m_compose_mode; }
};

} // namespace fkernel::drivers
