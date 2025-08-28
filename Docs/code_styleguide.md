# FKernel Code Style Guideline

FKernel follow four little rules to design inside himself:

1. Never, ever use assembly inline, prefer use external assembly like that 


```asm
global flush_idt

section .text 
bits 64 

flush_idt:
  lidt [rdi] ; Load the IDTR
  ret
```

and calling them in a external header, prefer use the directory `Include/Kernel/Arch/{your archicteture}/Cpu/Asm.h`

```cpp
#pragma once

#include <LibC/stdint.h>

/*
 * === Segments Flush
 */

extern "C" void flush_gdt(void* gdtr);
extern "C" void flush_idt(void* idtr);
extern "C" void flush_tss(LibC::uint16_t selector);

/*
 * === Invalid TLB Page
 */
extern "C" void invalid_tlb(LibC::uintptr_t addr);
```

> Avoid inline assembly to prevent hidden side effects, unportable instructions and poor compiler error messages. 
> External .asm files provide better modularity and ABI clarity

2. Every function need has your arguments being validated like that 

```cpp
void Manager::register_exception(int vector) noexcept
{
    FK::enforcef(vector >= 0 && vector <= 31,
        "IDT: Exception vector %d out of valid range [0..31]", vector);

    set_entry(vector, reinterpret_cast<void*>(exception_stubs[vector]), KERNEL_CODE_SELECTOR, IDT_TYPE_INTERRUPT_GATE, isr_ist[vector]);

    Logf(LogLevel::TRACE, "Register Exception Handler: Handler registered for Exception %u (%s)", vector, named_exception(vector));
}
```
> Validating arguments early avoids cascade failures, allowing the kernel to detect and isolate faults closer to their origin

3. Evit use if to change internal logic, instead this come routing the logic between functions like that

```cpp 
void PhysicalMemoryManager::mark_pages(PhysicalMemoryRegion& region, LibC::uint64_t page_index, LibC::uint64_t count, bool allocate) noexcept
{
    FK::enforcef(region.is_allocated(), "PMM: mark_pages called on unallocated region base=%p", region.base_addr);
    FK::enforcef(region.bitmap.is_valid(), "PMM: mark_pages called on region with invalid bitmap base=%p", region.base_addr);
    FK::enforcef(region.bitmap_allocated, "PMM: mark_pages called on region with unallocated bitmap base=%p", region.base_addr);
    FK::enforcef(page_index + count <= region.page_count, "PMM: mark_pages range out of bounds base=%p", region.base_addr);

    for (LibC::uint64_t i = 0; i < count; ++i) {
        if (allocate)
            region.mark_page(page_index + i);
        else
            region.unmark_page(page_index + i);
    }
}
```

> `if` is only allowed to route execution between distinct function calls.
> It must never be used for embedding complex logic, multiple expressions, side-effects, or conditionally executing blocks of logic.

4. Early Return is mandatory

To keep the performance of kernel at max speed, come cut out the all unnecessary logic if a condition is achieve, like that. 

```cpp
void PhysicalMemoryManager::free_page(LibC::uintptr_t phys_addr) noexcept
{
    FK::alert_if_f(phys_addr == 0, "PMM: free_page received null physical address");
    if (phys_addr == 0)
        return;

    auto* region = find_region(phys_addr);
    FK::alert_if_f(region == nullptr, "PMM: free_page failed to find region for address %p", phys_addr);
    if (!region)
        return;

    FK::alert_if_f(!is_valid_aligned_address(phys_addr, region->base_addr, region->page_count),
        "PMM: free_page received misaligned or out-of-range address %p", phys_addr);
    if (!is_valid_aligned_address(phys_addr, region->base_addr, region->page_count))
        return;

    ensure_bitmap_allocated(*region);
    mark_pages(*region, (phys_addr - region->base_addr) / TOTAL_MEMORY_PAGE_SIZE, 1, false);
}
```

> All functions must return early when validation fails or a terminating condition is met.
> This avoids nesting, improves cache predictability and simplifies control flow.

--- 

> [!NOTE]
> These rules are **not strictly enforced** in all cases, but **should always be followed when the context allows**.
> Clean code, debuggability, safety and performance are always prioritized over clever abstractions.
