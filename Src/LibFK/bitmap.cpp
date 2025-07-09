#include <LibFK/bitmap.h>
#include <LibFK/log.h>

namespace FK {

bool Bitmap::test(LibC::size_t bit) noexcept
{
    LibC::size_t idx = bit / 64;
    LibC::size_t offset = bit % 64;
    if (idx >= size_ / 64) {
        Logf(LogLevel::TRACE, "Bitmap: test out of range: bit=%lu (size=%lu)", bit, size_);
        return false;
    }
    bool result = (data_[idx] & (1ULL << offset)) != 0;
    Logf(LogLevel::TRACE, "Bitmap: test bit=%lu -> %d", bit, result);
    return result;
}

bool Bitmap::set(LibC::size_t bit) noexcept
{
    LibC::size_t idx = bit / 64;
    LibC::size_t offset = bit % 64;
    if (idx >= size_ / 64) {
        Logf(LogLevel::TRACE, "Bitmap: set out of range: bit=%lu (size=%lu)", bit, size_);
        return false;
    }
    data_[idx] |= (1ULL << offset);
    Logf(LogLevel::TRACE, "Bitmap: set bit=%lu", bit);
    return true;
}

bool Bitmap::clear(LibC::size_t bit) noexcept
{
    LibC::size_t idx = bit / 64;
    LibC::size_t offset = bit % 64;
    if (idx >= size_ / 64) {
        Logf(LogLevel::TRACE, "Bitmap: clear out of range: bit=%lu (size=%lu)", bit, size_);
        return false;
    }
    data_[idx] &= ~(1ULL << offset);
    Logf(LogLevel::TRACE, "Bitmap: clear bit=%lu", bit);
    return true;
}

bool Bitmap::find_first_clear(LibC::size_t& out_bit, LibC::size_t start) const noexcept
{
    Logf(LogLevel::TRACE, "Bitmap: find_first_clear starting at %lu", start);
    for (LibC::size_t i = start / 64; i < size_ / 64; ++i) {
        if (data_[i] != LibC::UINT64_MAX) {
            LibC::uint64_t inverted = ~data_[i];
            LibC::size_t bit_pos = __builtin_ctzll(inverted);
            out_bit = i * 64 + bit_pos;
            if (out_bit < size_) {
                Logf(LogLevel::TRACE, "Bitmap: find_first_clear found bit=%lu", out_bit);
                return true;
            }
        }
    }
    Log(LogLevel::TRACE, "Bitmap: find_first_clear no free bit found");
    return false;
}

void Bitmap::reset(LibC::uint64_t* buffer, LibC::size_t bits) noexcept
{
    Logf(LogLevel::TRACE, "Bitmap: reset buffer=%p bits=%lu", buffer, bits);
    if (!buffer || bits == 0) {
        data_ = nullptr;
        size_ = 0;
        Log(LogLevel::TRACE, "Bitmap: reset: invalid buffer or size, clearing state");
        return;
    }

    data_ = buffer;
    size_ = bits;
}
}
