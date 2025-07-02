#include <Kernel/MemoryManagement/PhysicalMemoryManagement/FreeBlocks.h>
#include <LibFK/log.h>

namespace MemoryManagement {
LibC::uint64_t FreeBlock::end_page() const noexcept
{
    return start_page + page_count;
}

bool FreeBlock::overlaps(FreeBlock const& other) const noexcept
{
    bool result = !(end_page() <= other.start_page || start_page >= other.end_page());
    /*
    Logf(LogLevel::TRACE, "PMM: overlap? A[%llu, %llu) B[%llu, %llu) => %s",
        start_page, end_page(),
        other.start_page, other.end_page(),
        result ? "true" : "false");
    */
    return result;
}

bool FreeBlock::adjacent_to(FreeBlock const& other) const noexcept
{
    bool result = end_page() == other.start_page || start_page == other.end_page();
    /*
    Logf(LogLevel::TRACE, "PMM: adjacent? A[%llu, %llu) B[%llu, %llu) => %s",
          start_page, end_page(),
          other.start_page, other.end_page(),
          result ? "true" : "false");
     */
    return result;
}

void FreeBlock::merge_with(FreeBlock const& other) noexcept
{
    /*
      Logf(LogLevel::TRACE, "PMM: merging A[%llu, %llu) with B[%llu, %llu)",
          start_page, end_page(),
          other.start_page, other.end_page());
  */
    LibC::uint64_t new_start = (start_page < other.start_page) ? start_page : other.start_page;
    LibC::uint64_t new_end = (end_page() > other.end_page()) ? end_page() : other.end_page();
    start_page = new_start;
    page_count = new_end - new_start;
    /*
        Logf(LogLevel::TRACE, "PMM: result = [%llu, %llu)", start_page, end_page());
      */
}

bool FreeBlock::operator<(FreeBlock const& other) const noexcept
{
    bool result = page_count < other.page_count;
    /*
    Logf(LogLevel::TRACE, "PMM: comparing A(size=%llu) < B(size=%llu) => %s",
        page_count, other.page_count,
        result ? "true" : "false");
  */
    return result;
}
}
