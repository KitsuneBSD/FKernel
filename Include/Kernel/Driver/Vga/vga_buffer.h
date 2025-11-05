#pragma once

#include <LibFK/Types/types.h>

namespace multiboot2 {
struct TagFramebuffer;
}

// VGA text-mode constants
constexpr uint16_t VGA_WIDTH = 80;
constexpr uint16_t VGA_HEIGHT = 25;

/**
 * @brief VGA color enumeration
 */
enum class Color : uint8_t {
  Black = 0,
  Blue,
  Green,
  Cyan,
  Red,
  Magenta,
  Brown,
  LightGray,
  DarkGray,
  LightBlue,
  LightGreen,
  LightCyan,
  LightRed,
  LightMagenta,
  Yellow,
  White
};

/**
 * @brief Create a VGA color byte from foreground and background colors
 */
constexpr uint8_t vga_entry_color(Color fg, Color bg) {
  return static_cast<uint8_t>(fg) | (static_cast<uint8_t>(bg) << 4);
}

/**
 * @brief Create a VGA text-mode entry combining a character and color
 */
constexpr uint16_t vga_entry(char c, uint8_t color) {
  return (static_cast<uint16_t>(color) << 8) | c;
}

// Helper: pack 8-bit r/g/b into framebuffer pixel according to mask/pos/size
static inline uint32_t pack_pixel_from_rgb(uint8_t r, uint8_t g, uint8_t b,
                                           uint8_t red_pos, uint8_t red_size,
                                           uint8_t green_pos,
                                           uint8_t green_size, uint8_t blue_pos,
                                           uint8_t blue_size) noexcept {
  uint32_t out = 0;

  if (red_size) {
    uint32_t rmax = (1u << red_size) - 1u;
    uint32_t rscaled =
        (static_cast<uint32_t>(r) * rmax + 127) / 255; // rounding
    out |= (rscaled & rmax) << red_pos;
  }
  if (green_size) {
    uint32_t gmax = (1u << green_size) - 1u;
    uint32_t gscaled = (static_cast<uint32_t>(g) * gmax + 127) / 255;
    out |= (gscaled & gmax) << green_pos;
  }
  if (blue_size) {
    uint32_t bmax = (1u << blue_size) - 1u;
    uint32_t bscaled = (static_cast<uint32_t>(b) * bmax + 127) / 255;
    out |= (bscaled & bmax) << blue_pos;
  }

  return out;
}

/**
 * @brief VGA text and framebuffer driver
 *
 * Supports both legacy text-mode (0xB8000) and modern framebuffer mode.
 * Provides basic text output, cursor control, scrolling, and color management.
 */
class vga {
private:
  /// RGBA representations of VGA colors
  static constexpr uint32_t vga_color_to_rgba[16] = {
      0x000000FF, // Black
      0x0000AAFF, // Blue
      0x00AA00FF, // Green
      0x00AAAAFF, // Cyan
      0xAA0000FF, // Red
      0xAA00AAFF, // Magenta
      0xAA5500FF, // Brown
      0xAAAAAAFF, // LightGray
      0x555555FF, // DarkGray
      0x5555FFFF, // LightBlue
      0x55FF55FF, // LightGreen
      0x55FFFFFF, // LightCyan
      0xFF5555FF, // LightRed
      0xFF55FFFF, // LightMagenta
      0xFFFF55FF, // Yellow
      0xFFFFFFFF  // White
  };

  /// Dual-mode driver: text-mode buffer or framebuffer
  enum class Mode { Text, Framebuffer } mode = Mode::Text;

  /// Legacy text-mode buffer at 0xB8000
  volatile uint16_t *const text_buffer =
      reinterpret_cast<volatile uint16_t *>(0xB8000);

  /// Framebuffer state
  uintptr_t fb_addr = 0;
  uint32_t fb_width = 0;
  uint32_t fb_height = 0;
  uint32_t fb_pitch = 0;
  uint8_t fb_bpp = 0;
  uint8_t fb_type = 0; ///< Framebuffer type from multiboot tag (0=indexed, 1=RGB, 2=EGA text)

  // Framebuffer color mask information
  uint8_t fb_red_pos = 0;
  uint8_t fb_red_size = 0;
  uint8_t fb_green_pos = 0;
  uint8_t fb_green_size = 0;
  uint8_t fb_blue_pos = 0;
  uint8_t fb_blue_size = 0;

  /// Current cursor position
  size_t row = 0;
  size_t col = 0;

  /// Current text color (foreground/background)
  uint8_t color = vga_entry_color(Color::LightGray, Color::Black);

  vga() = default; ///< Private constructor for singleton

  /// Scroll the text buffer up by one line
  void scroll();

  /// Update the hardware cursor
  void update_cursor();

  // Framebuffer helpers
  void fb_put_char(char c);
  void fb_clear();
  void fb_put_pixel(uint32_t x, uint32_t y, uint32_t rgba);
  uint32_t get_rgba_for_vga_color(Color vga_color) const noexcept;

public:
  /**
   * @brief Get the singleton instance of the VGA driver
   * @return Reference to the VGA driver
   */
  static vga &the() {
    static vga drv;
    return drv;
  }

  /**
   * @brief Initialize framebuffer mode from a Multiboot2 framebuffer tag
   * @param fb_tag Pointer to a Multiboot2 framebuffer tag
   * @return true on success, false otherwise
   */
  bool
  initialize_framebuffer(const multiboot2::TagFramebuffer *fb_tag) noexcept;

  /**
   * @brief Set the foreground and background colors for text
   * @param fg Foreground color
   * @param bg Background color
   */
  void set_color(Color fg, Color bg);

  /**
   * @brief Output a single character to the screen
   * @param c Character to print
   */
  void put_char(char c);

  /**
   * @brief Write a null-terminated ANSI string to the screen
   * @param str String to write
   */
  void write_ansi(const char *str);

  /**
   * @brief Write a null-terminated string to the screen
   * @param str String to write
   */
  void write(const char *str);

  /**
   * @brief Clear the screen
   */
  void clear();
};
