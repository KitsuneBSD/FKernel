#include <Kernel/Arch/x86_64/Segments/Gdt.h>
#include <Kernel/Arch/x86_64/Segments/Idt.h>
#include <Kernel/Boot/early_init.h>

void early_init(multiboot2::TagMemoryMap const& mmap)
{

    auto& gdt_manager
        = gdt::Manager::Instance();
    gdt_manager.initialize();

    auto& idt_manager = idt::Manager::Instance();
    idt_manager.initialize();
}
