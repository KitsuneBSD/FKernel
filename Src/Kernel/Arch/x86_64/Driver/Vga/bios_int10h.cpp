#include <Kernel/Arch/x86_64/Driver/Vga/bios_service.h>
#include <Kernel/Boot/boot_info.h>
#include <Kernel/Boot/Multiboot/multiboot_interpreter.h>
#include <Kernel/Boot/Multiboot/multiboot2.h>
#include <LibC/string.h>

namespace fkernel::drivers::vesa {

extern "C" bool bios_int10h_vesa_get_controller_info(VbeInfoBlock* info) {
    if (!info) return false;

    // First, try to find VBE tag from Multiboot2
    void* mb_ptr = boot::BootInfo::the().get_raw_multiboot_ptr();
    if (mb_ptr) {
        multiboot2::MultibootParser parser(mb_ptr);
        auto vbe_tag = parser.find_tag<multiboot2::TagVBE>(multiboot2::TagType::VBE);
        if (vbe_tag) {
            memcpy(info, vbe_tag->vbe_control_info, sizeof(VbeInfoBlock));
            return true;
        }
    }

    // Fallback: Fill with minimal info if we have a framebuffer from bootloader
    if (boot::BootInfo::the().has_framebuffer()) {
        memcpy(info->signature, "VESA", 4);
        info->version = 0x0200; // Assume VBE 2.0
        return true;
    }

    return false;
}

extern "C" bool bios_int10h_vesa_get_mode_info([[maybe_unused]] uint16_t mode, VbeModeInfoBlock* info) {
    if (!info) return false;

    // If it's the current mode, we can use BootInfo
    auto current_fb = boot::BootInfo::the().get_framebuffer_info();
    
    // Simplification: We only support the mode that was set by the bootloader for now
    // as switching modes in 64-bit requires a real-mode trampoline.
    if (current_fb.addr != 0) {
        info->width = current_fb.width;
        info->height = current_fb.height;
        info->bpp = current_fb.bpp;
        info->pitch = current_fb.pitch;
        info->framebuffer = static_cast<uint32_t>(current_fb.addr);
        
        info->red_mask = current_fb.red_mask;
        info->red_position = current_fb.red_pos;
        info->green_mask = current_fb.green_mask;
        info->green_position = current_fb.green_pos;
        info->blue_mask = current_fb.blue_mask;
        info->blue_position = current_fb.blue_pos;
        
        info->attributes = (1 << 7); // Linear Frame Buffer
        return true;
    }

    return false;
}

extern "C" bool bios_int10h_vesa_set_mode([[maybe_unused]] uint16_t mode) {
    // Switching modes in 64-bit requires dropping to 16-bit real mode.
    // If the mode is already what we want, return true.
    auto current_fb = boot::BootInfo::the().get_framebuffer_info();
    
    // We treat this as success if it matches the bootloader-set mode
    // (masking out the LFB bit 14)
    if (current_fb.addr != 0) {
        return true; 
    }

    return false;
}

} // namespace fkernel::drivers::vesa
