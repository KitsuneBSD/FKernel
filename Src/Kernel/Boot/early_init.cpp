#include <Kernel/Arch/x86_64/gdt.h>
#include <Kernel/Arch/x86_64/idt.h>
#include <Kernel/Boot/early_init.h>
#include <Kernel/Boot/multiboot2.h>

#include <LibFK/Log.hpp>

void early_init(multiboot2::TagMemoryMap)
{
    Log(LogLevel::INFO, "Init the kernel");
    gdt::Manager::instance().initialize();
    idt::Manager::instance().initialize();

    asm("ud2");
}
