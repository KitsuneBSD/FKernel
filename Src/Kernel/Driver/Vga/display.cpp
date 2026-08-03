#include <Kernel/Boot/Core/boot_info.h>
#include <Kernel/Driver/Vga/display.h>
#include <Kernel/Driver/Vga/display_text.h>
#include <Kernel/Driver/Vga/display_framebuffer.h>
#include <Kernel/Driver/Vga/font.h>
#include <Kernel/Memory/memory_manager.h>
#include <LibFK/Text/string.h>

#ifdef __x86_64
#include <Kernel/Arch/x86_64/io.h>
#endif

static Display *s_current_display = nullptr;

Display &Display::the() {
  static bool initialized = false;
  
  if (s_current_display)
    return *s_current_display;
  
  // Only initialize once
  if (initialized)
    return *s_current_display;
    
  initialized = true;
  
  // Don't initialize if memory manager not ready
  if (!MemoryManager::the().is_initialized()) {
    s_current_display = &DisplayText::the();
    return *s_current_display;
  }
  
  // Select display based on framebuffer availability
  if (boot::BootInfo::the().has_framebuffer()) {
    s_current_display = &DisplayFramebuffer::the();
  } else {
    s_current_display = &DisplayText::the();
  }

  return *s_current_display;
}

void Display::switch_to(Display &driver) {
  s_current_display = &driver;
  s_current_display->clear();
  fk::algorithms::klog("DISPLAY", "Switched to new display driver");
}