#pragma once

#include <LibFK/Types/types.h>
#include <LibFK/Core/Result.h>
#include <LibFK/Core/Error.h>
#include <Kernel/Arch/x86_64/Driver/Vga/vbe_types.h>
#include <Kernel/Driver/Vga/Types/framebuffer_info.h>

namespace fkernel::drivers::vesa {

/**
 * @class VESADriver
 * @brief Real VESA BIOS Extensions (VBE) Driver
 */
class VESADriver {
public:
    /** @return The singleton instance */
    static VESADriver& the();

    /**
     * @brief Set a specific VESA video mode
     * @param mode The VBE mode number (must include LFB bit 14)
     * @return Result of the operation
     */
    fk::core::Result<void, fk::core::Error> set_mode(uint16_t mode);

    /**
     * @brief Set video resolution by finding the best matching mode
     * @param width Desired width
     * @param height Desired height
     * @param bpp Desired bits per pixel
     * @return Result of the operation
     */
    fk::core::Result<void, fk::core::Error> set_resolution(uint32_t width, uint32_t height, uint32_t bpp);
    
    /** @return Pointer to the linear framebuffer (LFB) */
    uint8_t* get_framebuffer() const;
    
    uint32_t get_width() const { return m_mode_info.width; }
    uint32_t get_height() const { return m_mode_info.height; }
    uint32_t get_pitch() const { return m_mode_info.pitch; }
    uint32_t get_bpp() const { return m_mode_info.bpp; }

    /** @return True if VBE is available on this hardware */
    bool is_available() const { return m_vbe_supported; }

    /** @return Current framebuffer info structure */
    FramebufferInfo get_info() const;

private:
    VESADriver();
    
    /** @brief Discover VBE capabilities via INT 10h AX=4F00h */
    void discover_vbe();

    /** @brief Fetch mode information via INT 10h AX=4F01h */
    fk::core::Result<void, fk::core::Error> get_mode_info(uint16_t mode, VbeModeInfoBlock* block);

    VbeInfoBlock m_controller_info{};
    VbeModeInfoBlock m_mode_info{};
    bool m_vbe_supported{false};
};

} // namespace fkernel::drivers::vesa

// Maintain backward compatibility for existing code using vesa namespace
namespace vesa {
    using VESADriver = fkernel::drivers::vesa::VESADriver;
}