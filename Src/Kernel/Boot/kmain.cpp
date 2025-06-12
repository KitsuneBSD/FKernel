#include "Boot/kmain.h"
#include "LibFK/Log.h"

void kmain(uint32_t multiboot_magic, void *mb_info) {
  using namespace vga;
  using namespace multiboot2;

  console.clear();
  console.set_color(Color::LightGreen, Color::Black);

  console.write("Multiboot Magic: ");
  console.write_hex(multiboot_magic);
  console.write("\n");

  if (multiboot_magic != BOOTLOADER_MAGIC) {
    Log(LogLevel::ERROR, "Invalid multiboot magic number!");
    while (true) {
      __asm__("hlt");
    }
  }

  const auto *info = reinterpret_cast<const Info *>(mb_info);
  MultibootParser mb_parser(info);
  // Cmdline
  if (auto *cmdline = mb_parser.find_tag<TagString>(TagType::Cmdline)) {
    Log(LogLevel::INFO, "Boot cmdline detected:");
    console.write(cmdline->get_string());
    console.write("\n");
  } else {
    Log(LogLevel::WARN, "No boot cmdline found.");
  }

  // Memory Map
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

  Log(LogLevel::INFO, "Total usable memory:");
  console.write_dec(total_mem_mb);
  console.write(" MB\n");

  Log(LogLevel::INFO, "Kernel initialized successfully.");
  while (true) {
    __asm__("hlt");
  }
}
