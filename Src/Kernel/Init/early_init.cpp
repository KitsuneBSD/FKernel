#include <Kernel/Boot/init.h>
#include <Kernel/Arch/x86_64/Segments/Gdt.h>
#include <Kernel/Arch/x86_64/Segments/Idt.h>
#include <Kernel/Boot/early_init.h>
#include <LibFK/log.h>
#include <LibFK/types.h>

#include <Kernel/Driver/Pit/Pit.h>

void early_init(multiboot2::TagMemoryMap const& mmap)
{
  UNUSED(mmap)

    auto& gdt_manager
        = gdt::Manager::instance();
    gdt_manager.initialize();

    auto& idt_manager = idt::Manager::Instance();
    idt_manager.initialize();

    Pit::Instance().initialize(1000);

    init();
}
