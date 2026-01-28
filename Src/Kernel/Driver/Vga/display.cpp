#include <Kernel/Boot/boot_info.h>
#include <Kernel/Driver/Vga/display.h>
#include <Kernel/Driver/Vga/display_framebuffer.h>
#include <Kernel/Driver/Vga/font.h>
#include <Kernel/Memory/memory_manager.h>
#include <LibFK/Text/string.h>

#ifdef __x86_64
#include <Kernel/Arch/x86_64/io.h>
#endif

static Display *s_current_display = nullptr;

Display &Display::the() {
  static bool initializing = false;

  // Allow VESA fallback if memory is ready, otherwise use text mode
  if (!MemoryManager::the().is_initialized() && initializing) {
    return DisplayText::the(); // Memory not ready, use VGA
  }
  
  if (s_current_display)
    return *s_current_display;
  
  // Don't early initialize - let init() handle display selection
  if (!MemoryManager::the().is_initialized()) {
    return DisplayText::the(); // Safe fallback for early boot
  }
  
  initializing = true;
  if (boot::BootInfo::the().has_framebuffer()) {
    s_current_display = &DisplayFramebuffer::the();
  } else {
    s_current_display = &DisplayText::the();
  }
  initializing = false;

  return *s_current_display;
}

void Display::switch_to(Display &driver) {
  s_current_display = &driver;
  s_current_display->clear();
  fk::algorithms::klog("DISPLAY", "Switched to new display driver");
}