#include <Kernel/Boot/kmain.h>

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
        if (entry->type == 1) { // Memory available
            total_mem += entry->length;
        }
    }

    LibC::uint64_t total_mem_mb = total_mem / static_cast<LibC::uint64_t>(1024 * 1024);

    // TODO: Pass a MemoryMap instead total_mem
    // early_init(total_mem);

    // FIXME: Infinite loop with hlt block the system, adding another alternatives
    // first
    while (true) {
        __asm__("hlt");
    }
}
