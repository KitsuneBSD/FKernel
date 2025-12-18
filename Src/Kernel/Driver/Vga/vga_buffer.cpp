// DEPRECATED: This file is no longer used.
// 
// The VGA display system has been refactored to support both BIOS and EFI boot modes:
// 
// - New implementation: display.h / display.cpp
//   - display_text: BIOS text-mode (0xB8000)
//   - display_efi: EFI framebuffer (pixel-based rendering)
// 
// - Compatibility wrapper: vga_adapter.h
//   - vga class: Maintains backward compatibility
//   - Automatically selects display backend based on boot mode
// 
// See Include/Kernel/Driver/Vga/display.h for the new implementation.

#include <Kernel/Driver/Vga/display.h>

