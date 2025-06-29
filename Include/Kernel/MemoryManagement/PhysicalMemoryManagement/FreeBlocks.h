#pragma once

#include <LibC/stddef.h>
#include <LibC/stdint.h>

namespace MemoryManagement {
struct FreeBlock {
    LibC::uint64_t start_page; // Índice da primeira página
    LibC::uint64_t page_count; // Número de páginas contíguas

    LibC::uint64_t end_page() const noexcept;
    bool overlaps(FreeBlock const& other) const noexcept;
    bool adjacent_to(FreeBlock const& other) const noexcept;
    void merge_with(FreeBlock const& other) noexcept;

    bool operator<(FreeBlock const& other) const noexcept;
};

static void swap_blocks(FreeBlock& a, FreeBlock& b) noexcept
{
    FreeBlock tmp = a;
    a = b;
    b = tmp;
}

static void quicksort_blocks(FreeBlock* arr, LibC::size_t left, LibC::size_t right) noexcept
{
    if (left >= right)
        return;

    LibC::size_t i = left;
    LibC::size_t j = right;
    FreeBlock pivot = arr[left + (right - left) / 2];

    while (i <= j) {
        while (arr[i].start_page < pivot.start_page)
            i++;
        while (arr[j].start_page > pivot.start_page)
            j--;
        if (i <= j) {
            swap_blocks(arr[i], arr[j]);
            i++;
            if (j > 0)
                j--;
        }
    }

    if (j > left)
        quicksort_blocks(arr, left, j);
    if (i < right)
        quicksort_blocks(arr, i, right);
}
}
