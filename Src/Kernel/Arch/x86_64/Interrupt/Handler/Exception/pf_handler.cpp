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
    if (task && !task->control.lifecycle.is_a_kernel_task) {
        // Check if address is in valid user ranges
        bool valid = false;
        
        // 1. Heap range
        if (cr2 >= task->resources.memory.regions.heap_start && cr2 < task->resources.memory.regions.heap_break) {
            valid = true;
        }
        // 2. Mmap range
        else if (cr2 >= task->resources.memory.regions.mmap_start && cr2 < task->resources.memory.regions.mmap_end) {
            valid = true;
        }
        // 3. User stack (expand range to cover typical user stack locations)
        else if (cr2 >= 0x7ffffff00000ULL && cr2 < 0x7fffffffe000ULL) {
            valid = true;
        }

        if (valid) {
            uintptr_t vaddr = cr2 & ~0xFFFULL;
            uintptr_t phys = PhysicalMemoryManager::the().alloc_page();
            
            fk::algorithms::kdebug("DEMAND PAGING", "Task %lu: Mapping page %p -> %p (CR2=%p, RIP=%p)", 
                                   task->control.identity.id.value(), (void*)vaddr, (void*)phys, (void*)cr2, (void*)frame->rip);

            // Map as RW User
            VirtualMemoryManager::the().map_page(vaddr, phys, PageFlags::Present | PageFlags::Writable | PageFlags::User);
            
            // Zero the page
            memset(reinterpret_cast<void*>(vaddr), 0, 0x1000);
            
            return;
        }
    }

    fk::algorithms::kexception(
        "Page Fault",
        "vector=%u error=0x%lx (%s, %s, %s %s) RIP=%p RSP=%p CR2=%p PID=%lu",
        (unsigned)vector, (uint64_t)frame->error_code,
        (frame->error_code & 1) ? "Present" : "Not Present",
        (frame->error_code & 2) ? "Write" : "Read",
        (frame->error_code & 4) ? "User" : "Kernel",
        (frame->error_code & 16) ? "Instruction Fetch" : "Data Access",
        (void*)frame->rip, (void*)frame->rsp, (void*)cr2,
        task ? task->control.identity.id.value() : 0
    );

    if (task && !task->control.lifecycle.is_a_kernel_task && (frame->error_code & 4)) {
        fk::algorithms::kerror("PF", "User-mode Page Fault. Killing process %lu", task->control.identity.id.value());
        SchedulerManager::the().terminate_current(-11); // SIGSEGV = 11
    }

    halt_forever();
}