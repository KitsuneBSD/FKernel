#pragma once

#include "LibC/stddef.h"
#include <LibC/stdint.h>

namespace FK {

class Bitmap {
private:
    LibC::uint64_t* data_;
    LibC::size_t size_;

public:
    Bitmap(LibC::uint64_t* buffer, LibC::size_t bits) noexcept
        : data_(buffer)
        , size_(bits)
    {
    }

    bool test(LibC::size_t bit) noexcept;
    bool set(LibC::size_t bit) noexcept;
    bool clear(LibC::size_t bit) noexcept;
    bool find_first_clear(LibC::size_t& out_bit, LibC::size_t start = 0) const noexcept;
};

}
