#pragma once

#include <LibFK/Types/types.h>
#include <Kernel/Driver/Vga/Types/framebuffer_info.h>

namespace fkernel {

class RenderSurface {
public:
    RenderSurface() = default;
    ~RenderSurface();

    void initialize(const FramebufferInfo& info);
    void allocate_back_buffer();
    void allocate_saved_buffer();
    void free_all();

    void clear(uint32_t pixel);
    void clear_rect(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint32_t pixel);
    void copy_rect(uint32_t sx, uint32_t sy, uint32_t dx, uint32_t dy, uint32_t w, uint32_t h);
    void blit_tile(uint32_t tx, uint32_t ty, uint32_t tile_size);

    uint8_t* active_buffer() const;
    void save();
    void restore();

private:
    FramebufferInfo m_info;
    uint8_t* m_back_buffer{nullptr};
    uint8_t* m_saved_buffer{nullptr};
    bool m_double_buffering{false};

    void fill_memory(uint8_t* start, uint32_t pixel);
};

} // namespace fkernel
