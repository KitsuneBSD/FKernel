#include <Kernel/Driver/Vga/display.h>
#include <Kernel/Driver/Vga/font.h>
#include <Kernel/Boot/boot_info.h>
#include <Kernel/Memory/memory_manager.h>
#include <LibFK/Text/string.h>

#ifdef __x86_64
#include <Kernel/Arch/x86_64/io.h>
#endif

static Display* s_current_display = nullptr;

Display &Display::the() {
  static bool initializing = false;

  // Se o MemoryManager ainda não subiu, ou se já estamos inicializando o display,
  // forçamos o modo texto (safe fallback).
  if (!MemoryManager::the().is_initialized() || initializing) {
      return DisplayText::the();
  }

  if (s_current_display) return *s_current_display;

  initializing = true;
  if (boot::BootInfo::the().has_framebuffer()) {
    s_current_display = &DisplayFramebuffer::the();
  } else {
    s_current_display = &DisplayText::the();
  }
  initializing = false;

  return *s_current_display;
}

void Display::switch_to(Display& driver) {
    s_current_display = &driver;
    s_current_display->clear();
    fk::algorithms::klog("DISPLAY", "Switched to new display driver");
}
