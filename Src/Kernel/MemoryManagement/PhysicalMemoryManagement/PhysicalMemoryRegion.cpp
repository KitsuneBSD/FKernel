#include <Kernel/MemoryManagement/PhysicalMemoryManagement/PhysicalMemoryRegion.h>
#include <LibFK/bitmap.h>

namespace MemoryManagement {
bool PhysicalMemoryRegion::is_page_used(LibC::uint64_t page_index) noexcept
{
    return bitmap.test(page_index);
}

void PhysicalMemoryRegion::mark_page(LibC::uint64_t page_index) noexcept
{
    bitmap.set(page_index);
}

void PhysicalMemoryRegion::unmark_page(LibC::uint64_t page_index) noexcept
{
    bitmap.clear(page_index);
}

bool PhysicalMemoryRegion::find_free_page(LibC::uint64_t& out_page_index, LibC::uint64_t start) const noexcept
{
    return bitmap.find_first_clear(out_page_index, start);
}

bool PhysicalMemoryRegion::is_allocated() const noexcept
{
    return allocated;
}
}
