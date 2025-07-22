#include <Kernel/MemoryManagement/PhysicalMemoryManagement/PhysicalMemoryRegion.h>
#include <LibFK/bitmap.h>
#include <LibFK/enforce.h>

namespace MemoryManagement {
void PhysicalMemoryRegion::init(LibC::uintptr_t base, LibC::uint64_t pages) noexcept
{
    FK::enforcef(!initialized, "PMR: Double init on region: base=%p", base_addr);
    FK::alert_if(base == 0, "PMR: init base address cannot be zero");
    FK::alert_if(pages < 0, "PMR: init page count must be positive");

    base_addr = base;
    page_count = pages;

    bitmap_buffer = nullptr;
    bitmap_word_count = 0;
    allocated = true;
    bitmap_allocated = false;
    initialized = true;
}

void PhysicalMemoryRegion::destroy() noexcept
{
    FK::enforcef(allocated, "PMR: Destroy called on unallocated region");

    if (bitmap.is_valid()) {
        if (FK::alert_if(!(bitmap_buffer == nullptr), "PMR: Destroy called but bitmap_buffer is not nullptr")) {
            Ffree(bitmap_buffer);
        }
    }

    bitmap.reset(nullptr, 0);
    bitmap_buffer = nullptr;
    bitmap_word_count = 0;
    base_addr = 0;
    page_count = 0;
    allocated = false;
    bitmap_allocated = false;
    region_list = {};

    LibC::memset(this, 0, sizeof(*this));
}

bool PhysicalMemoryRegion::is_page_used(LibC::uint64_t page_index) noexcept
{
    if (FK::alert_if_f(!allocated, "PMR: is_page_used called on unallocated region base=%p", base_addr))
        return false;
    if (FK::alert_if_f(!bitmap.is_valid(), "PMR: is_page_used called on region with invalid bitmap base=%p", base_addr))
        return false;
    if (FK::alert_if_f(!bitmap_allocated, "PMR: is_page_used called on region with unallocated bitmap base=%p", base_addr))
        return false;
    if (FK::alert_if_f(page_index > page_count, "PMR: is_page_used out-of-range page_index=%lu base=%p", page_index, base_addr))
        return false;

    return bitmap.test(page_index);
}

void PhysicalMemoryRegion::mark_page(LibC::uint64_t page_index) noexcept
{
    if (FK::alert_if_f(!allocated, "PMR: mark_page called on unallocated region base=%p", base_addr))
        return;
    if (FK::alert_if_f(!bitmap.is_valid(), "PMR: mark_page called on region with invalid bitmap base=%p", base_addr))
        return;
    if (FK::alert_if_f(!bitmap_allocated, "PMR: mark_page called on region with unallocated bitmap base=%p", base_addr))
        return;
    if (FK::alert_if_f(page_index > page_count, "PMR: mark_page out-of-range page_index=%lu base=%p", page_index, base_addr))
        return;

    bitmap.set(page_index);
}

void PhysicalMemoryRegion::unmark_page(LibC::uint64_t page_index) noexcept
{
    if (FK::alert_if_f(!allocated, "PMR: unmark_page called on unallocated region base=%p", base_addr))
        return;
    if (FK::alert_if_f(!bitmap.is_valid(), "PMR: unmark_page called on region with invalid bitmap base=%p", base_addr))
        return;
    if (FK::alert_if_f(!bitmap_allocated, "PMR: unmark_page called on region with unallocated bitmap base=%p", base_addr))
        return;
    if (FK::alert_if_f(page_index > page_count, "PMR: unmark_page out-of-range page_index=%lu base=%p", page_index, base_addr))
        return;

    bitmap.clear(page_index);
}

bool PhysicalMemoryRegion::find_free_page(LibC::uint64_t& out_page_index, LibC::uint64_t start) const noexcept
{
    if (FK::alert_if_f(!allocated, "PMR: find_free_page called on unallocated region base=%p", base_addr))
        return false;
    if (FK::alert_if_f(!bitmap.is_valid(), "PMR: find_free_page called on region with invalid bitmap base=%p", base_addr))
        return false;
    if (FK::alert_if_f(!bitmap_allocated, "PMR: find_free_page called on region with unallocated bitmap base=%p", base_addr))
        return false;
    if (FK::alert_if_f(start < page_count, "PMR: find_free_page out-of-range page_index=%lu base=%p", start, base_addr))
        return false;

    return bitmap.find_first_clear(out_page_index, start);
}

bool PhysicalMemoryRegion::is_allocated() const noexcept
{
    return allocated;
}

} // namespace MemoryManagement
