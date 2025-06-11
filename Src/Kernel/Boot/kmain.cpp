#include "Boot/kmain.h"
#include <Boot/multiboot2.h>
#include <Driver/Vga_Buffer.hpp>

void kmain(uint32_t multiboot_magic, void *mb_info) {
  using namespace multiboot2;
  using namespace vga;

  // TODO: Create a function kernel::log to make
  console.clear();
  console.set_color(Color::LightGreen, Color::Black);
  console.write_hex(multiboot_magic);
  if (multiboot_magic != BOOTLOADER_MAGIC) {
    console.set_color(Color::LightRed, Color::White);
    console.write("Error: Invalid multiboot magic number!\n");
    while (true) {
      __asm__("hlt");
    }
  }

  const Info *info = reinterpret_cast<const Info *>(mb_info);

  if (auto *cmdline = reinterpret_cast<const TagString *>(
          info->find_tag(TagType::Cmdline))) {
    console.set_color(Color::White, Color::Black);
    console.write("Boot cmdline: ");
    console.write(cmdline->get_string());
    console.write("\n");
  } else {
    console.set_color(Color::Yellow, Color::Black);
    console.write("No boot cmdline found.\n");
  }

  auto *mem_map_tag =
      reinterpret_cast<const TagMemoryMap *>(info->find_tag(TagType::MMap));
  if (!mem_map_tag) {
    console.set_color(Color::LightRed, Color::Black);
    console.write("Error: Memory map tag not found!\n");
    while (true) {
      __asm__("hlt");
    }
  }

  uint64_t total_mem = 0;
  for (const auto *entry = mem_map_tag->begin(); entry != mem_map_tag->end();
       ++entry) {
    if (entry->type == 1) { // 1 = memória disponível conforme spec Multiboot2
      total_mem += entry->length;
    }
  }

  console.write_hex(total_mem);

  uint64_t total_mem_mb = total_mem / (1024 * 1024);
  console.write("Total available memory: ");
  console.write_dec(total_mem_mb);
  console.write(" MB\n");

  console.set_color(Color::LightGreen, Color::Black);
  console.write("Kernel initialized successfully.\n");

  while (true) {
    __asm__("hlt");
  }
}
