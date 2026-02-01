#pragma once

#include <LibFK/Types/types.h>

namespace fkernel {

class DirtyTracker {
public:
    static constexpr uint32_t TILE_SIZE = 32;

    DirtyTracker() = default;
    ~DirtyTracker();

    void initialize(uint32_t width, uint32_t height);
    void mark_dirty(uint32_t x, uint32_t y, uint32_t w, uint32_t h);
    bool is_tile_dirty(uint32_t tx, uint32_t ty) const;
    void mark_clean(uint32_t tx, uint32_t ty);
    void mark_all_clean();
    void reset();

    uint32_t tiles_x() const { return m_tiles_x; }
    uint32_t tiles_y() const { return m_tiles_y; }

private:
    uint32_t m_tiles_x{0};
    uint32_t m_tiles_y{0};
    uint8_t* m_bitset{nullptr};
};

} // namespace fkernel
