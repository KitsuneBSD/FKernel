#include <Kernel/Boot/early_init.h>
#include <Kernel/Boot/kmain.h>
#include <Kernel/Driver/VgaBuffer.h>

#include <LibFK/Log.hpp>

void kmain(LibC::uint32_t multiboot_magic, void* mb_info)
{
    using namespace multiboot2;

    if (multiboot_magic != BOOTLOADER_MAGIC) {
        while (true) {
            __asm__("hlt");
        }
    }

    auto const info = reinterpret_cast<Info const*>(mb_info);
    MultibootParser mb_parser(info);

    auto* mem_map_tag = mb_parser.find_tag<TagMemoryMap>(TagType::MMap);
    if (!mem_map_tag) {
        while (true) {
            __asm__("hlt");
        }
    }

    console.set_color(vga::Color::LightGreen, vga::Color::Black);
    console.clear();

#ifdef FKERNEL_DEBUG
    Logger::Instance().SetLevel(LogLevel::TRACE);
#else
    Logger::Instance().SetLevel(LogLevel::INFO);
#endif

    early_init(*mem_map_tag);

    // NOTE: This infinite loop is the end point of kernel, our goal is never reach here
    while (true) {
        __asm__("hlt");
    }
}
