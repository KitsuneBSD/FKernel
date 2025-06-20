#include <Kernel/Boot/kmain.h>
#include <Kernel/Driver/VgaBuffer.h>

void kmain(LibC::uint32_t multiboot_magic, void* mb_info)
{
    using namespace multiboot2;

    if (multiboot_magic != BOOTLOADER_MAGIC) {
        while (true) {
            __asm__("hlt");
        }
    }

    auto const* info = reinterpret_cast<Info const*>(mb_info);
    MultibootParser mb_parser(info);

    auto const* mem_map_tag = mb_parser.find_tag<TagMemoryMap>(TagType::MMap);
    if (!mem_map_tag) {
        while (true) {
            __asm__("hlt");
        }
    }

    LibC::uint64_t total_mem = 0;
    for (auto const* entry = mem_map_tag->begin(); entry != mem_map_tag->end();
         ++entry) {
        if (entry->type == 1) {
            total_mem += entry->length;
        }
    }

    console.set_color(vga::Color::LightGreen, vga::Color::Black);
    console.clear();
    console.write("Hello World!");

    // NOTE: This infinite loop is the end point of kernel, our goal is never reach here
    while (true) {
        __asm__("hlt");
    }
}
