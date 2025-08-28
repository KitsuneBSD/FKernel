#include <LibC/string.h>
#include <LibFK/bitmap.h>
#include <LibFK/enforce.h>
#include <LibFK/log.h>

namespace FK {
bool Bitmap::test(LibC::size_t bit) noexcept
{
    if (FK::alert_if_f(bit > size_, "Bitmap: test out of range: bit=%lu (size=%lu)", bit, size_))
        return false;

    LibC::size_t idx = bit / 64;
    LibC::size_t offset = bit % 64;

    bool result = (data_[idx] & (1ULL << offset)) != 0;
    return result;
}

bool Bitmap::set(LibC::size_t bit) noexcept
{
    if (FK::alert_if_f(bit > size_, "Bitmap: set out of range: bit=%lu (size=%lu)", bit, size_))
        return false;

    LibC::size_t idx = bit / 64;
    LibC::size_t offset = bit % 64;

    data_[idx] |= (1ULL << offset);
    return true;
}

bool Bitmap::clear(LibC::size_t bit) noexcept
{
    if (FK::alert_if_f(bit > size_, "Bitmap: clear out of range: bit=%lu (size=%lu)", bit, size_))
        return false;

    LibC::size_t idx = bit / 64;
    LibC::size_t offset = bit % 64;

    data_[idx] &= ~(1ULL << offset);
    return true;
}

bool Bitmap::find_first_clear(LibC::size_t& out_bit, LibC::size_t start) const noexcept
{
    if (FK::alert_if_f(start > size_, "Bitmap: find_first_clear start out of range: start=%lu (size=%lu)", start, size_))
        return false;

    LibC::size_t word_count = (size_ + 63) / 64;

    for (LibC::size_t i = start / 64; i < word_count; ++i) {
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
    Log(LogLevel::WARN, "Bitmap: find_first_clear no free bit found");
    return false;
}

void Bitmap::reset(LibC::uint64_t* buffer, LibC::size_t bits) noexcept
{
    if (alert_if(buffer == nullptr, "Bitmap: reset called with nullptr buffer"))
        return;
    if (alert_if(bits <= 0, "Bitmap: reset called with zero bits"))
        return;

    data_ = buffer;
    size_ = bits;

    LibC::size_t word_count = (bits + 63) / 64;

    LibC::memset(data_, 0, word_count * sizeof(LibC::uint64_t));
}
}
