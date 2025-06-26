#include <LibC/stdint.h>
#include <LibFK/Log.h>

#include <Kernel/Boot/multiboot2.h>
#include <Kernel/Boot/multiboot_interpreter.h>

#include <Kernel/Boot/early_init.h>

#include <Kernel/Driver/SerialPort.h>

extern "C" void kmain(LibC::uint32_t multiboot2_magic, void* multiboot_ptr)
{
    if (multiboot2_magic != multiboot2::BOOTLOADER_MAGIC) {
        Logf(LogLevel::ERROR, "Received magic %d is different than %d", multiboot2_magic, multiboot2::BOOTLOADER_MAGIC);
        while (true) {
            __asm__("hlt");
        }
    }

    multiboot2::MultibootParser mb_parser(multiboot_ptr);
    Logger::Instance().SetLevel(LogLevel::TRACE);

    auto mem_map_tag = mb_parser.find_tag<multiboot2::TagMemoryMap>(multiboot2::TagType::MMap);

    if (!mem_map_tag) {
        Log(LogLevel::ERROR, "Memory Map not found");
        while (true) {

            __asm__("hlt");
        }
    }
    early_init(*mem_map_tag);

    // Infinite Loop Hang
    while (true) {
        asm("hlt");
    }
}
