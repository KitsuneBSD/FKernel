#pragma once

#include "LibC/stddef.h"
#include <LibC/stdint.h>

namespace FK {

class Bitmap {
private:
    LibC::uint64_t* data_;
    LibC::size_t size_;

public:
    Bitmap() noexcept
        : data_(nullptr)
        , size_(0)
    {
    }

    Bitmap(LibC::uint64_t* buffer, LibC::size_t bits) noexcept
        : data_(buffer)
        , size_(bits)
    {
    }

    bool is_valid() const noexcept { return data_ != nullptr; }
    LibC::size_t size() const noexcept { return size_; }

    bool test(LibC::size_t bit) noexcept;
    bool set(LibC::size_t bit) noexcept;
    bool clear(LibC::size_t bit) noexcept;
    bool find_first_clear(LibC::size_t& out_bit, LibC::size_t start = 0) const noexcept;

    void reset(LibC::uint64_t* buffer, LibC::size_t bits) noexcept;
};

}
