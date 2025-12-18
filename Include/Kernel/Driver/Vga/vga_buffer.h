#pragma once

// This header is deprecated. Use display.h and vga_adapter.h instead.
// The old vga class is now a compatibility wrapper over the new display abstraction.
// It automatically selects between BIOS text mode and EFI framebuffer modes.

#include <Kernel/Driver/Vga/vga_adapter.h>
#include <Kernel/Driver/Vga/display.h>

// Re-export Color enum and helper functions for compatibility
using Color = ::Color;

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