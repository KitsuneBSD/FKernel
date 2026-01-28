#include <Kernel/Arch/x86_64/Driver/Vga/vesa.h>
#include <LibFK/Algorithms/log.h>
#include <LibC/string.h>

namespace vesa {

VESADriver& VESADriver::the() {
    static VESADriver instance;
    return instance;
}

VESADriver::VESADriver() {
    find_vbe();
}

void VESADriver::find_vbe() {
    fk::algorithms::klog("VESA", "Initializing VBE discovery...");
    
    // Em hardware real, chamaríamos INT 10h AX=4F00h aqui.
    // Simulando o preenchimento da VbeInfoBlock
    memcpy(m_vbe_info.signature, "VESA", 4);
    m_vbe_info.version = 0x0300; // VBE 3.0
    m_vbe_supported = true;

    fk::algorithms::klog("VESA", "VBE version %d.%d detected", 
                         m_vbe_info.version >> 8, m_vbe_info.version & 0xFF);
}

fk::core::Result<void, fk::core::Error> VESADriver::set_mode(uint16_t mode) {
    if (!m_vbe_supported) return fk::core::Error::NotImplemented;

    fk::algorithms::klog("VESA", "Setting VESA mode 0x%x...", mode);
    
    // Aqui chamaríamos INT 10h AX=4F02h
    // Simulando preenchimento da m_mode_info para um modo 1024x768x32
    if (mode == 0x4117) { // Exemplo de modo 1024x768x32
        m_mode_info.width = 1024;
        m_mode_info.height = 768;
        m_mode_info.bpp = 32;
        m_mode_info.pitch = 1024 * 4;
        m_mode_info.framebuffer = 0xFD000000; // Endereço físico fictício de LFB
        
        m_mode_info.red_mask = 8;
        m_mode_info.red_position = 16;
        m_mode_info.green_mask = 8;
        m_mode_info.green_position = 8;
        m_mode_info.blue_mask = 8;
        m_mode_info.blue_position = 0;

        fk::algorithms::klog("VESA", "Mode 0x%x set: %ux%ux%u at %p", 
                             mode, m_mode_info.width, m_mode_info.height, 
                             m_mode_info.bpp, (void*)(uintptr_t)m_mode_info.framebuffer);
        return {};
    }

    return fk::core::Error::NotFound;
}

fk::core::Result<void, fk::core::Error> VESADriver::set_resolution(uint32_t width, uint32_t height, uint32_t bpp) {
    if (!m_vbe_supported) return fk::core::Error::NotImplemented;

    fk::algorithms::klog("VESA", "Searching for mode: %ux%ux%u", width, height, bpp);

    // Lista simplificada de modos comuns para simulação
    struct {
        uint32_t w, h, b;
        uint16_t mode;
    } common_modes[] = {
        {1024, 768, 32, 0x4117},
        {800, 600, 32, 0x4114},
        {1280, 1024, 32, 0x411B}
    };

    for (const auto& m : common_modes) {
        if (m.w == width && m.h == height && m.b == bpp) {
            fk::algorithms::klog("VESA", "Found matching mode: 0x%x", m.mode);
            return set_mode(m.mode);
        }
    }

    fk::algorithms::kwarn("VESA", "No matching mode found for %ux%ux%u", width, height, bpp);
    return fk::core::Error::NotFound;
}

} // namespace vesa