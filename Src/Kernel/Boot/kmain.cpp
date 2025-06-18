#include "Boot/kmain.h"
#include "Init/early_init.h"
#include "LibFK/Log.h"

void kmain(uint32_t multiboot_magic, void *mb_info) {
  using namespace vga;
  using namespace multiboot2;

#ifdef FKERNEL_DEBUG
  Logger::Instance().SetLevel(LogLevel::TRACE);
#else
  Logger::Instance().SetLevel(LogLevel::INFO);
#endif

  console.clear();
  console.set_color(Color::LightGreen, Color::Black);

  Logf(LogLevel::TRACE, "Multiboot Magic: %d\n", multiboot_magic);

  // TODO: Start module threatment and kernel args in multiboot
  if (multiboot_magic != BOOTLOADER_MAGIC) {
    Log(LogLevel::ERROR, "Invalid multiboot magic number!");
    while (true) {
      __asm__("hlt");
    }
  }

  const auto *info = reinterpret_cast<const Info *>(mb_info);
  MultibootParser mb_parser(info);
  if (auto *cmdline = mb_parser.find_tag<TagString>(TagType::Cmdline)) {
    Logf(LogLevel::TRACE, "Boot cmdline detected: %s\n", cmdline->get_string());
  } else {
    Log(LogLevel::WARN, "No boot cmdline found.");
  }

  const auto *mem_map_tag = mb_parser.find_tag<TagMemoryMap>(TagType::MMap);
  if (!mem_map_tag) {
    Log(LogLevel::ERROR, "Memory map tag not found!");
    while (true) {
      __asm__("hlt");
    }
  }

  uint64_t total_mem = 0;
  for (const auto *entry = mem_map_tag->begin(); entry != mem_map_tag->end();
       ++entry) {
    if (entry->type == 1) { // Memory available
      total_mem += entry->length;
    }
  }

  uint64_t total_mem_mb = total_mem / (1024 * 1024);

  Logf(LogLevel::TRACE, "Total usable memory: %d MB\n", total_mem_mb);

  // TODO: Pass a MemoryMap instead total_mem
  early_init(total_mem);

  Log(LogLevel::INFO, "Kernel initialized successfully.");

  // FIXME: Infinite loop with hlt block the system, adding another alternatives
  // first
  while (true) {
    __asm__("hlt");
  }
}
