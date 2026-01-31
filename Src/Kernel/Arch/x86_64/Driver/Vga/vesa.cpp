#include <Kernel/Arch/x86_64/Driver/Vga/vesa.h>
#include <Kernel/Memory/memory_manager.h>
#include <LibFK/Algorithms/log.h>
#include <LibC/string.h>

namespace fkernel::drivers::vesa {

/**
 * @brief BIOS Bridge for Real Mode Interrupts
 * Note: In a real x86_64 kernel, this usually requires dropping to 
 * compatibility mode or using a pre-allocated low memory buffer 
 * and a 16-bit trampoline.
 */
extern "C" bool bios_int10h_vesa_get_controller_info(VbeInfoBlock* info);
extern "C" bool bios_int10h_vesa_get_mode_info(uint16_t mode, VbeModeInfoBlock* info);
extern "C" bool bios_int10h_vesa_set_mode(uint16_t mode);

VESADriver& VESADriver::the() {
    static VESADriver instance;
    return instance;
}

VESADriver::VESADriver() {
    discover_vbe();
}

void VESADriver::discover_vbe() {
    fk::algorithms::klog("VESA", "Discovering VBE controller...");
    
    // Prepare signature for VESA 2.0+
    memcpy(m_controller_info.signature, "VBE2", 4);

    // Call BIOS bridge
    if (!bios_int10h_vesa_get_controller_info(&m_controller_info)) {
        fk::algorithms::kwarn("VESA", "VBE not supported or BIOS call failed");
        m_vbe_supported = false;
        return;
    }

    if (memcmp(m_controller_info.signature, "VESA", 4) != 0) {
        fk::algorithms::kwarn("VESA", "Invalid VBE signature");
        m_vbe_supported = false;
        return;
    }

    m_vbe_supported = true;
    fk::algorithms::klog("VESA", "VBE %d.%d detected", 
                         m_controller_info.version >> 8, 
                         m_controller_info.version & 0xFF);

    // Enumerate modes
    uint32_t mode_ptr = m_controller_info.video_mode_ptr;
    uint16_t* mode_list = reinterpret_cast<uint16_t*>(((mode_ptr >> 16) << 4) + (mode_ptr & 0xFFFF));
    
    m_supported_modes.clear();
    for (int i = 0; mode_list[i] != 0xFFFF && i < 256; i++) {
        m_supported_modes.push_back(mode_list[i]);
    }

    fk::algorithms::klog("VESA", "Found %zu supported modes", m_supported_modes.size());
}

fk::core::Result<void, fk::core::Error> VESADriver::get_mode_info(uint16_t mode, VbeModeInfoBlock* block) {
    if (!m_vbe_supported) return fk::core::Error::DeviceError;
    
    if (bios_int10h_vesa_get_mode_info(mode, block)) {
        return {};
    }
    
    return fk::core::Error::IOError;
}

fk::core::Result<void, fk::core::Error> VESADriver::set_mode(uint16_t mode) {
    if (!m_vbe_supported) return fk::core::Error::DeviceError;

    // We must ensure bit 14 is set for Linear Frame Buffer (LFB)
    uint16_t target_mode = mode | (1 << 14);

    fk::algorithms::klog("VESA", "Fetching info for mode 0x%x...", target_mode);
    
    VbeModeInfoBlock new_info{};
    auto res = get_mode_info(target_mode, &new_info);
    if (res.is_error()) return res.error();

    fk::algorithms::klog("VESA", "Setting VBE mode 0x%x (%ux%ux%u)...", 
                         target_mode, new_info.width, new_info.height, new_info.bpp);

    if (!bios_int10h_vesa_set_mode(target_mode)) {
        fk::algorithms::kerror("VESA", "Failed to set video mode 0x%x", target_mode);
        return fk::core::Error::IOError;
    }

    m_mode_info = new_info;
    
    // Map the LFB if MemoryManager is ready
    if (MemoryManager::the().is_initialized() && m_mode_info.framebuffer != 0) {
        uintptr_t lfb_phys = m_mode_info.framebuffer;
        size_t lfb_size = m_mode_info.height * m_mode_info.pitch;
        
        fk::algorithms::klog("VESA", "Mapping LFB: %p, size: %zu bytes", (void*)lfb_phys, lfb_size);
        
        // Map pages for the framebuffer
        for (uintptr_t offset = 0; offset < lfb_size; offset += 0x1000) {
            MemoryManager::the().map_page(lfb_phys + offset, lfb_phys + offset, 
                                          PageFlags::Present | PageFlags::Writable | PageFlags::CacheDisabled);
        }
    }

    return {};
}

fk::core::Result<void, fk::core::Error> VESADriver::set_resolution(uint32_t width, uint32_t height, uint32_t bpp) {
    if (!m_vbe_supported) return fk::core::Error::DeviceError;

    fk::algorithms::klog("VESA", "Searching for best mode: %ux%ux%u", width, height, bpp);

    for (uint16_t m : m_supported_modes) {
        VbeModeInfoBlock info{};
        // Try getting info for both standard and LFB version of the mode
        uint16_t target_mode = m | (1 << 14);
        if (get_mode_info(target_mode, &info).is_ok()) {
            if (info.width == width && info.height == height && info.bpp == bpp) {
                if (info.attributes & (1 << 7)) { // Ensure LFB is supported
                    return set_mode(m);
                }
            }
        }
    }

    return fk::core::Error::NotFound;
}

uint8_t* VESADriver::get_framebuffer() const {
    return reinterpret_cast<uint8_t*>(static_cast<uintptr_t>(m_mode_info.framebuffer));
}

FramebufferInfo VESADriver::get_info() const {
    return {
        .address = get_framebuffer(),
        .width = get_width(),
        .height = get_height(),
        .pitch = get_pitch(),
        .bpp = get_bpp()
    };
}

} // namespace fkernel::drivers::vesa
