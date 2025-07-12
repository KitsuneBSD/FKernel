#include <Kernel/MemoryManagement/PhysicalMemoryManagement/PhysicalMemoryRegion.h>
#include <LibFK/bitmap.h>

namespace MemoryManagement {
void PhysicalMemoryRegion::init(LibC::uintptr_t base, LibC::uint64_t pages) noexcept
{
    if (allocated) {
        Logf(LogLevel::WARN, "PMR: Double init on region: base=0x%lx", base_addr);
        return;
    }

    base_addr = base;
    page_count = pages;

    bitmap_buffer = nullptr;
    bitmap_word_count = 0;
    allocated = true;
    bitmap_allocated = false;
}

void PhysicalMemoryRegion::destroy() noexcept
{
    if (!allocated) {
        Log(LogLevel::WARN, "PMR: Destroy called on unallocated region");
        return;
    }

    if (bitmap.is_valid() && bitmap_buffer) {
        Ffree(bitmap_buffer);
    }

    bitmap.reset(nullptr, 0);
    bitmap_buffer = nullptr;
    bitmap_word_count = 0;
    base_addr = 0;
    page_count = 0;
    allocated = false;
    bitmap_allocated = false;
    region_list = {}; // limpa lista interna, caso tenha sido usada
}

bool PhysicalMemoryRegion::is_page_used(LibC::uint64_t page_index) noexcept
{
    if (!allocated) {
        Logf(LogLevel::WARN, "PMR: is_page_used called on unallocated region base=0x%lx", base_addr);
        return true;
    }
    if (!bitmap.is_valid()) {
        Logf(LogLevel::WARN, "PMR: is_page_used called on region with invalid bitmap base=0x%lx", base_addr);
        return true;
    }

    if (!bitmap_allocated) {
        Logf(LogLevel::WARN, "PMR: is_page_used called on region with unallocated bitmap base=0x%lx", base_addr);
        return true;
    }

    if (page_index >= page_count) {
        Logf(LogLevel::WARN, "PMR: is_page_used out-of-range page_index=%lu base=0x%lx", page_index, base_addr);
        return true;
    }

    return bitmap.test(page_index);
}

void PhysicalMemoryRegion::mark_page(LibC::uint64_t page_index) noexcept
{
    if (!allocated) {
        Logf(LogLevel::WARN, "PMR: mark_page called on unallocated region base=0x%lx", base_addr);
        return;
    }

    if (!bitmap.is_valid()) {
        Logf(LogLevel::WARN, "PMR: mark_page called on region with invalid bitmap base=0x%lx", base_addr);
        return;
    }

    if (!bitmap_allocated) {
        Logf(LogLevel::WARN, "PMR: mark_page called on region with unallocated bitmap base=0x%lx", base_addr);
        return;
    }

    if (page_index >= page_count) {
        Logf(LogLevel::WARN, "PMR: mark_page called with out-of-range page_index=%lu on base=0x%lx", page_index, base_addr);
        return;
    }

    bitmap.set(page_index);
}

void PhysicalMemoryRegion::unmark_page(LibC::uint64_t page_index) noexcept
{
    if (!allocated) {
        Logf(LogLevel::WARN, "PMR: unmark_page called on unallocated region base=0x%lx", base_addr);
        return;
    }
    if (!bitmap.is_valid()) {
        Logf(LogLevel::WARN, "PMR: unmark_page called on region with invalid bitmap base=0x%lx", base_addr);
        return;
    }

    if (!bitmap_allocated) {
        Logf(LogLevel::WARN, "PMR: unmark_page called on region with unallocated bitmap base=0x%lx", base_addr);
        return;
    }

    if (page_index >= page_count) {
        Logf(LogLevel::WARN, "PMR: unmark_page out-of-range page_index=%lu base=0x%lx", page_index, base_addr);
        return;
    }

    bitmap.clear(page_index);
}

bool PhysicalMemoryRegion::find_free_page(LibC::uint64_t& out_page_index, LibC::uint64_t start) const noexcept
{
    if (!allocated) {
        Logf(LogLevel::WARN, "PMR: find_free_page called on unallocated region base=0x%lx", base_addr);
        return false;
    }
    if (!bitmap.is_valid()) {
        Logf(LogLevel::WARN, "PMR: find_free_page called on region with invalid bitmap base=0x%lx", base_addr);
        return false;
    }

    if (!bitmap_allocated) {
        Logf(LogLevel::WARN, "PMR: find_free_page called on region with unallocated bitmap base=0x%lx", base_addr);
        return false;
    }

    if (start >= page_count) {
        Logf(LogLevel::WARN, "PMR: find_free_page out-of-range start=%lu base=0x%lx", start, base_addr);
        return false;
    }

    return bitmap.find_first_clear(out_page_index, start);
}

bool PhysicalMemoryRegion::is_allocated() const noexcept
{
    return allocated;
}
}
