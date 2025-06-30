#include <Kernel/Arch/x86_64/Segments/Gdt.h>
#include <Kernel/Arch/x86_64/Segments/Idt.h>
#include <Kernel/Boot/early_init.h>
#include <Kernel/MemoryManagement/PhysicalMemoryManagement/PhysicalMemoryManager.h>
#include <Kernel/MemoryManagement/VirtualMemoryManagement/VirtualMemoryManagement.h>

#include <Kernel/Driver/Pit.h>

extern "C" LibC::uintptr_t current_pml4_ptr;

void early_init(multiboot2::TagMemoryMap const& mmap)
{

    auto& gdt_manager
        = gdt::Manager::Instance();
    gdt_manager.initialize();

    auto& idt_manager = idt::Manager::Instance();
    idt_manager.initialize();

    Pit::initialize(1000);
    auto& pmm = MemoryManagement::PhysicalMemoryManager::instance();
    pmm.initialize(mmap);
    auto& vmm = MemoryManagement::VirtualMemoryManager::instance();
    vmm.initialize();
}
