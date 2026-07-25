#include <Kernel/Arch/x86_64/Driver/Vga/vbe_types.h>
#include <LibFK/Core/result.h>
#include <LibFK/Core/error.h>
#include <LibFK/Container/vector.h>
#include <Kernel/Driver/Vga/Types/framebuffer_info.h>

namespace fkernel::drivers::vesa {

class VESADriver {
public:
    static VESADriver& the();

    void discover_vbe();
    fk::core::Result<void, fk::core::Error> set_mode(uint16_t mode);
    fk::core::Result<void, fk::core::Error> set_resolution(uint32_t width, uint32_t height, uint32_t bpp);
    
    fk::core::Result<void, fk::core::Error> get_mode_info(uint16_t mode, VbeModeInfoBlock* block);

    uint8_t* get_framebuffer() const;
    FramebufferInfo get_info() const;

    uint32_t get_width() const { return m_mode_info.width; }
    uint32_t get_height() const { return m_mode_info.height; }
    uint32_t get_pitch() const { return m_mode_info.pitch; }
    uint16_t get_bpp() const { return m_mode_info.bpp; }

    bool is_available() const { return m_vbe_supported; }

    const fk::containers::Vector<uint16_t>& modes() const { return m_supported_modes; }

private:
    VESADriver();
    
    VbeInfoBlock m_controller_info{};
    VbeModeInfoBlock m_mode_info{};
    fk::containers::Vector<uint16_t> m_supported_modes;
    bool m_vbe_supported{false};
};

} // namespace fkernel::drivers::vesa

// Maintain backward compatibility for existing code using vesa namespace
namespace vesa {
    using VESADriver = fkernel::drivers::vesa::VESADriver;
}