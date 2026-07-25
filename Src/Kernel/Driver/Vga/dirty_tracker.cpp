#include <Kernel/Driver/Vga/dirty_tracker.h>
#include <Kernel/Memory/memory_manager.h>
#include <LibFK/Utilities/memory.h>

namespace fkernel {

DirtyTracker::~DirtyTracker() { reset(); }

void DirtyTracker::initialize(uint32_t width, uint32_t height) {
    reset();
    m_tiles_x = (width + TILE_SIZE - 1) / TILE_SIZE;
    m_tiles_y = (height + TILE_SIZE - 1) / TILE_SIZE;
    size_t size = (m_tiles_x * m_tiles_y + 7) / 8;
    m_bitset = static_cast<uint8_t*>(MemoryManager::the().allocate(size));
    if (m_bitset) fk::memory::set(m_bitset, 0xFF, size); // Start fully dirty
}

void DirtyTracker::mark_dirty(uint32_t x, uint32_t y, uint32_t w, uint32_t h) {
    if (!m_bitset) return;
    uint32_t start_tx = x / TILE_SIZE;
    uint32_t start_ty = y / TILE_SIZE;
    uint32_t end_tx = (x + w + TILE_SIZE - 1) / TILE_SIZE;
    uint32_t end_ty = (y + h + TILE_SIZE - 1) / TILE_SIZE;

    if (end_tx > m_tiles_x) end_tx = m_tiles_x;
    if (end_ty > m_tiles_y) end_ty = m_tiles_y;

    for (uint32_t ty = start_ty; ty < end_ty; ++ty) {
        for (uint32_t tx = start_tx; tx < end_tx; ++tx) {
            uint32_t idx = ty * m_tiles_x + tx;
            m_bitset[idx / 8] |= (1 << (idx % 8));
        }
    }
}

bool DirtyTracker::is_tile_dirty(uint32_t tx, uint32_t ty) const {
    if (!m_bitset) return false;
    uint32_t idx = ty * m_tiles_x + tx;
    return m_bitset[idx / 8] & (1 << (idx % 8));
}

void DirtyTracker::mark_clean(uint32_t tx, uint32_t ty) {
    if (!m_bitset) return;
    uint32_t idx = ty * m_tiles_x + tx;
    m_bitset[idx / 8] &= ~(1 << (idx % 8));
}

void DirtyTracker::mark_all_clean() {
    if (!m_bitset) return;
    fk::memory::set(m_bitset, 0, (m_tiles_x * m_tiles_y + 7) / 8);
}

void DirtyTracker::reset() {
    if (m_bitset) {
        MemoryManager::the().free(m_bitset);
        m_bitset = nullptr;
    }
    m_tiles_x = 0; m_tiles_y = 0;
}

} // namespace fkernel
