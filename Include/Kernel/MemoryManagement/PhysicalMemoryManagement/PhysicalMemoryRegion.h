#pragma once

#include <Kernel/MemoryManagement/FreeListAllocator/falloc.h>
#include <LibC/stddef.h>
#include <LibC/stdint.h>
#include <LibFK/bitmap.h>
#include <LibFK/intrusiveList.h>
#include <LibFK/log.h>
#include <LibFK/types.h>

namespace MemoryManagement {

struct PhysicalMemoryRegion {
    LibC::uintptr_t base_addr = static_cast<LibC::uintptr_t>(1 * FK::MiB);
    LibC::uint64_t page_count = 0;

    LibC::uint64_t* bitmap_buffer = nullptr;
    FK::Bitmap bitmap;

    LibC::size_t bitmap_word_count = 0;

    bool bitmap_allocated = false;
    bool allocated = false;
    bool initialized = false;
    FK::IntrusiveNode<PhysicalMemoryRegion> list_node;
    PhysicalMemoryRegion() noexcept = default;

    PhysicalMemoryRegion(LibC::uintptr_t base, LibC::uint64_t pages) noexcept
        : base_addr(base)
        , page_count(pages)
    {
    }

    void init(LibC::uintptr_t base, LibC::uint64_t pages) noexcept;
    void destroy() noexcept;
    bool is_page_used(LibC::uint64_t page_index) noexcept;
    void mark_page(LibC::uint64_t page_index) noexcept;
    void unmark_page(LibC::uint64_t page_index) noexcept;
    bool find_free_page(LibC::uint64_t& out_page_index, LibC::uint64_t start = 0) const noexcept;
    bool is_allocated() const noexcept;
};

}
