#include <LibFK/bitmap.h>
#include <LibFK/log.h>

namespace FK {
bool Bitmap::test(LibC::size_t bit) noexcept
{
    LibC::size_t idx = bit / 64;
    LibC::size_t offset = bit % 64;
    if (idx >= size_ / 64)
        return false;
    return (data_[idx] & (1ULL << offset)) != 0;
}

bool Bitmap::set(LibC::size_t bit) noexcept
{
    LibC::size_t idx = bit / 64;
    LibC::size_t offset = bit % 64;
    if (idx >= size_ / 64)
        return false;
    data_[idx] |= (1ULL << offset);
    return true;
}

bool Bitmap::clear(LibC::size_t bit) noexcept
{
    LibC::size_t idx = bit / 64;
    LibC::size_t offset = bit % 64;
    if (idx >= size_ / 64)
        return false;
    data_[idx] &= ~(1ULL << offset);
    return true;
}

bool Bitmap::find_first_clear(LibC::size_t& out_bit, LibC::size_t start) const noexcept
{
    for (LibC::size_t i = start / 64; i < size_ / 64; ++i) {
        if (data_[i] != LibC::UINT64_MAX) {
            LibC::uint64_t inverted = ~data_[i];
            LibC::size_t bit_pos = __builtin_ctzll(inverted);
            out_bit = i * 64 + bit_pos;
            if (out_bit < size_)
                return true;
        }
    }
    return false;
}
}
