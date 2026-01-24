#include <Kernel/Arch/x86_64/Interrupt/Handler/exception_macros.h>
#include <Kernel/Scheduler/scheduler.h>
#include <Kernel/Memory/VirtualMemory/virtual_memory_manager.h>
#include <Kernel/Memory/PhysicalMemory/physical_memory_manager.h>
#include <Kernel/Memory/VirtualMemory/Pages/page_flags.h>
#include <LibFK/Algorithms/log.h>
#include <LibC/string.h>

void page_fault_handler(uint8_t vector, InterruptFrame* frame) {
    uint64_t cr2;
    asm volatile("mov %%cr2, %0" : "=r"(cr2));

    auto* task = SchedulerManager::the().current();
    if (task && !task->is_a_kernel_task) {
        // Check if address is in valid user ranges
        bool valid = false;
        
        // 1. Heap range
        if (cr2 >= task->memory_regions.heap_start && cr2 < task->memory_regions.heap_break) {
            valid = true;
        }
        // 2. Mmap range
        else if (cr2 >= task->memory_regions.mmap_start && cr2 < task->memory_regions.mmap_end) {
            valid = true;
        }
        // 3. User stack (including child processes)
        else if (cr2 >= 0x700000 && cr2 < 0x900000) {
            valid = true;
        }

        if (valid) {
            uintptr_t vaddr = cr2 & ~0xFFFULL;
            uintptr_t phys = PhysicalMemoryManager::the().alloc_page();
            
            // Map as RW User
            VirtualMemoryManager::the().map_page(vaddr, phys, PageFlags::Present | PageFlags::Writable | PageFlags::User);
            
            // Zero the page
            memset(reinterpret_cast<void*>(vaddr), 0, 0x1000);
            
            // fk::algorithms::klog("DEMAND PAGING", "Mapped user page: %p -> %p", (void*)vaddr, (void*)phys);
            
            return;
        }
    }

    fk::algorithms::kexception(
        "Page Fault",
        "vector=%u error=0x%lx (%s, %s, %s %s) RIP=%p RSP=%p CR2=%p",
        (unsigned)vector, (uint64_t)frame->error_code,
        (frame->error_code & 1) ? "Present" : "Not Present",
        (frame->error_code & 2) ? "Write" : "Read",
        (frame->error_code & 4) ? "User" : "Kernel",
        (frame->error_code & 16) ? "Instruction Fetch" : "Data Access",
        (void*)frame->rip, (void*)frame->rsp, (void*)cr2
    );

    halt_forever();
}
