#include <Kernel/Boot/multiboot_interpreter.h>
#include <Kernel/MemoryManagement/BumpAllocator/bump_alloc.h>
#include <Kernel/MemoryManagement/PhysicalMemoryManagement/FreeBlocks.h>
#include <Kernel/MemoryManagement/PhysicalMemoryManagement/PhysicalMemoryManager.h>
#include <LibC/stddef.h>
#include <LibC/stdint.h>
#include <LibFK/Log.h>

extern LibC::uintptr_t __heap_start;
extern LibC::uintptr_t __heap_end;

namespace MemoryManagement {

void PhysicalMemoryManager::initialize(multiboot2::TagMemoryMap const& mmap) noexcept
{
    total_page_size = 4096;

    BumpAllocator::instance().initialize(reinterpret_cast<LibC::uintptr_t>(&__heap_start), reinterpret_cast<LibC::uintptr_t>(&__heap_end));
    Log(LogLevel::INFO, "PMM: Initializing Physical Memory Manager");

    LibC::uint64_t lowest_addr = ~0ULL;
    LibC::uint64_t highest_addr = 0;
    for (auto it = mmap.begin(); it < mmap.end(); ++it) {
        auto const& entry = *it;
        Logf(LogLevel::TRACE, "PMM: Memory region: base=0x%016llX size=0x%016llX type=%u",
            entry.base_addr, entry.length, static_cast<LibC::uint64_t>(entry.type));

        if (multiboot2::is_available(entry.type)) {
            if (entry.base_addr < lowest_addr)
                lowest_addr = entry.base_addr;
            if (entry.base_addr + entry.length > highest_addr)
                highest_addr = entry.base_addr + entry.length;
        }
    }

    if (lowest_addr >= highest_addr) {
        Log(LogLevel::ERROR, "PMM: No available memory regions found");
        return;
    }

    pmm_base_addr = lowest_addr;
    pmm_total_pages = (highest_addr - pmm_base_addr) / total_page_size;
    pmm_bitmap_size = (pmm_total_pages + 63) / 64;

    Logf(LogLevel::TRACE, "PMM: Base address = 0x%016llX", pmm_base_addr);
    Logf(LogLevel::TRACE, "PMM: Total pages = %llu", pmm_total_pages);
    Logf(LogLevel::TRACE, "PMM: Bitmap size (uint64_t words) = %zu", pmm_bitmap_size);

    // Aloca bitmap primeiro

    pmm_bitmap = static_cast<LibC::uint64_t*>(
        Balloc_zeroed(pmm_bitmap_size * sizeof(LibC::uint64_t), alignof(LibC::uint64_t)));
    if (!pmm_bitmap) {
        Log(LogLevel::ERROR, "PMM: Failed to allocate bitmap memory");
        return;
    }

    LibC::memset(pmm_bitmap, 0xFF, pmm_bitmap_size * sizeof(LibC::uint64_t));

    for (auto it = mmap.begin(); it < mmap.end(); ++it) {
        auto const& entry = *it;
        if (multiboot2::is_available(entry.type)) {
            mark_region_free(entry.base_addr, entry.length);
        }
    }

    free_block_count = 0;
    LibC::uint64_t current_start = 0;
    LibC::uint64_t current_len = 0;

    for (LibC::uint64_t i = 0; i < pmm_total_pages; ++i) {
        if (!get_bit(i)) { // Página livre
            if (current_len == 0)
                current_start = i;
            ++current_len;
        } else if (current_len > 0) {
            add_free_block(current_start, current_len);
            current_len = 0;
        }
    }
    if (current_len > 0)
        add_free_block(current_start, current_len);

    consolidate_blocks();

    // Conta as páginas livres efetivas
    LibC::uint64_t free_pages = 0;
    for (LibC::size_t i = 0; i < free_block_count; ++i)
        free_pages += free_blocks[i].page_count;

    LibC::uint64_t total_memory = pmm_total_pages * total_page_size;
    LibC::uint64_t free_memory = free_pages * total_page_size;

    Logf(LogLevel::INFO,
        "PMM: Physical memory: total = %llu KiB (%llu MiB), free = %llu KiB (%llu MiB)",
        total_memory / 1024, total_memory / 1024 / 1024,
        free_memory / 1024, free_memory / 1024 / 1024);
    Log(LogLevel::INFO, "PMM: Initialization complete");
}

void PhysicalMemoryManager::set_bit(LibC::size_t index) noexcept
{
    if (index >= pmm_total_pages)
        return;
    LibC::size_t word = index / 64;
    LibC::size_t bit = index % 64;
    pmm_bitmap[word] |= (1ULL << bit);
}

void PhysicalMemoryManager::clear_bit(LibC::size_t index) noexcept
{
    if (index >= pmm_total_pages)
        return;
    LibC::size_t word = index / 64;
    LibC::size_t bit = index % 64;
    pmm_bitmap[word] &= ~(1ULL << bit);
}

bool PhysicalMemoryManager::get_bit(LibC::size_t index) noexcept
{
    if (index >= pmm_total_pages)
        return true; // Fora do bitmap = usado (segurança)
    LibC::size_t word = index / 64;
    LibC::size_t bit = index % 64;
    return (pmm_bitmap[word] & (1ULL << bit)) != 0;
}

void PhysicalMemoryManager::mark_region_used(LibC::uint64_t base, LibC::uint64_t size) noexcept
{
    if (base < pmm_base_addr) {
        Logf(LogLevel::WARN, "PMM: mark_region_used base 0x%llx < base_addr 0x%llx ignored", base, pmm_base_addr);
        return;
    }

    LibC::uint64_t start_page = (base - pmm_base_addr) / total_page_size;
    LibC::uint64_t end_page = ((base + size + total_page_size - 1) - pmm_base_addr) / total_page_size;

    if (end_page > pmm_total_pages)
        end_page = pmm_total_pages;

    Logf(LogLevel::TRACE, "PMM: mark_region_used base=0x%016llX size=0x%016llX start_page=%llu end_page=%llu",
        base, size, start_page, end_page);

    for (LibC::uint64_t page = start_page; page < end_page; ++page)
        set_bit(page);
}

void PhysicalMemoryManager::mark_region_free(LibC::uint64_t base, LibC::uint64_t size) noexcept
{
    if (base < pmm_base_addr) {
        Logf(LogLevel::WARN, "PMM: mark_region_free base 0x%llx < base_addr 0x%llx ignored", base, pmm_base_addr);
        return;
    }

    LibC::uint64_t start_page = (base - pmm_base_addr) / total_page_size;
    LibC::uint64_t end_page = ((base + size + total_page_size - 1) - pmm_base_addr) / total_page_size;

    if (end_page > pmm_total_pages)
        end_page = pmm_total_pages;

    Logf(LogLevel::TRACE, "PMM: mark_region_free base=0x%016llX size=0x%016llX start_page=%llu end_page=%llu",
        base, size, start_page, end_page);

    for (LibC::uint64_t page = start_page; page < end_page; ++page)
        clear_bit(page);
}

void PhysicalMemoryManager::add_free_block(LibC::uint64_t start_page, LibC::uint64_t page_count) noexcept
{
    if (free_block_count >= max_free_blocks) {
        Log(LogLevel::ERROR, "PMM: add_free_block failed: max blocks reached");
        return;

        FreeBlock new_block { start_page, page_count };

        free_blocks[free_block_count++] = new_block;

        Logf(LogLevel::TRACE, "PMM: add_free_block [%llu, %llu)", start_page, start_page + page_count);
    }
}

void PhysicalMemoryManager::consolidate_blocks() noexcept
{
    if (free_block_count == 0)
        return;

    quicksort_blocks(free_blocks, 0, free_block_count - 1);

    LibC::size_t i = 0;
    while (i + 1 < free_block_count) {
        if (free_blocks[i].adjacent_to(free_blocks[i + 1]) || free_blocks[i].overlaps(free_blocks[i + 1])) {
            Logf(LogLevel::TRACE, "PMM: consolidating blocks [%llu, %llu) + [%llu, %llu)",
                free_blocks[i].start_page, free_blocks[i].end_page(),
                free_blocks[i + 1].start_page, free_blocks[i + 1].end_page());
            free_blocks[i].merge_with(free_blocks[i + 1]);

            for (LibC::size_t k = i + 1; k + 1 < free_block_count; ++k)
                free_blocks[k] = free_blocks[k + 1];
            --free_block_count;
        } else {
            ++i;
        }
    }
}

LibC::uintptr_t PhysicalMemoryManager::alloc_page() noexcept
{
    for (LibC::size_t word_idx = 0; word_idx < pmm_bitmap_size; ++word_idx) {
        if (pmm_bitmap[word_idx] != ~0ULL) {
            LibC::uint64_t free_word = ~pmm_bitmap[word_idx];
            LibC::size_t bit_pos = __builtin_ctzl(free_word);
            LibC::size_t page = word_idx * 64 + bit_pos;
            if (page >= pmm_total_pages)
                break;

            set_bit(page);
            Logf(LogLevel::TRACE, "PMM: alloc_page => page %zu (addr=0x%llx)", page, pmm_base_addr + page * total_page_size);
            return pmm_base_addr + page * total_page_size;
        }
    }
    Log(LogLevel::ERROR, "PMM: Out of physical memory");
    return 0;
}

void PhysicalMemoryManager::free_page(LibC::uintptr_t phys_addr) noexcept
{
    if (phys_addr < pmm_base_addr)
        return;

    LibC::uint64_t page = (phys_addr - pmm_base_addr) / total_page_size;
    if (page >= pmm_total_pages)
        return;

    clear_bit(page);
    Logf(LogLevel::TRACE, "PMM: free_page => page %llu (addr=0x%llx)", page, phys_addr);

    add_free_block(page, 1);
    consolidate_blocks();
}

LibC::uintptr_t PhysicalMemoryManager::alloc_contiguous_pages(LibC::uint64_t count) noexcept
{
    LibC::size_t best_index = static_cast<LibC::size_t>(-1);
    LibC::uint64_t best_size = ~0ULL;

    for (LibC::size_t i = 0; i < free_block_count; ++i) {
        if (free_blocks[i].page_count >= count && free_blocks[i].page_count < best_size) {
            best_index = i;
            best_size = free_blocks[i].page_count;
        }
    }

    if (best_index == static_cast<LibC::size_t>(-1)) {
        Log(LogLevel::ERROR, "PMM: No suitable contiguous block found");
        return 0;
    }

    LibC::uint64_t start_page = free_blocks[best_index].start_page;

    for (LibC::uint64_t i = 0; i < count; ++i)
        set_bit(start_page + i);

    Logf(LogLevel::TRACE, "PMM: alloc_contiguous_pages count=%llu at page %llu (addr=0x%llx)",
        count, start_page, pmm_base_addr + start_page * total_page_size);

    if (free_blocks[best_index].page_count == count) {
        for (LibC::size_t j = best_index; j + 1 < free_block_count; ++j)
            free_blocks[j] = free_blocks[j + 1];
        --free_block_count;
    } else {
        free_blocks[best_index].start_page += count;
        free_blocks[best_index].page_count -= count;
    }

    return pmm_base_addr + start_page * total_page_size;
}

void PhysicalMemoryManager::free_contiguous_pages(LibC::uintptr_t phys_addr, LibC::uint64_t count) noexcept
{
    if (phys_addr < pmm_base_addr)
        return;

    LibC::uint64_t start_page = (phys_addr - pmm_base_addr) / total_page_size;
    if (start_page + count > pmm_total_pages)
        return;

    for (LibC::uint64_t i = 0; i < count; ++i)
        clear_bit(start_page + i);

    Logf(LogLevel::TRACE, "PMM: free_contiguous_pages count=%llu at page %llu (addr=0x%llx)",
        count, start_page, phys_addr);

    add_free_block(start_page, count);
    consolidate_blocks();
}
}
