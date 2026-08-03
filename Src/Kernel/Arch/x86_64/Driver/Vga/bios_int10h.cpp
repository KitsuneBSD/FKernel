#include <Kernel/Arch/x86_64/Driver/Vga/bios_service.h>
#include <Kernel/Boot/Core/boot_info.h>
#include <Kernel/Boot/Multiboot/multiboot_interpreter.h>
#include <Kernel/Boot/Multiboot/multiboot2.h>
#include <LibFK/Utilities/memory.h>

namespace fkernel::drivers::vesa {

extern "C" bool bios_int10h_vesa_get_controller_info(VbeInfoBlock* info) {
    if (!info) return false;

    // First, try to find VBE tag from Multiboot2
    void* mb_ptr = boot::BootInfo::the().get_raw_multiboot_ptr();
    if (mb_ptr) {
        multiboot2::MultibootParser parser(mb_ptr);
        auto vbe_tag = parser.find_tag<multiboot2::TagVBE>(multiboot2::TagType::VBE);
        if (vbe_tag) {
            fk::memory::copy(info, vbe_tag->vbe_control_info, sizeof(VbeInfoBlock));
            return true;
        }
    }

    // Fallback: Fill with minimal info if we have a framebuffer from bootloader
    if (boot::BootInfo::the().has_framebuffer()) {
        fk::memory::copy(info->signature, "VESA", 4);
        info->version = 0x0200; // Assume VBE 2.0
        return true;
    }

    return false;
}

extern "C" bool bios_int10h_vesa_get_mode_info(uint16_t mode, VbeModeInfoBlock* info) {
    if (!info) return false;

    // Remove LFB bit for comparison/lookup
    uint16_t real_mode = mode & ~(1 << 14);

    // 1. Try to find VBE mode info from Multiboot2 if it's the current mode
    void* mb_ptr = boot::BootInfo::the().get_raw_multiboot_ptr();
    if (mb_ptr) {
        multiboot2::MultibootParser parser(mb_ptr);
        auto vbe_tag = parser.find_tag<multiboot2::TagVBE>(multiboot2::TagType::VBE);
        if (vbe_tag && vbe_tag->vbe_mode == real_mode) {
            fk::memory::copy(info, vbe_tag->vbe_mode_info, sizeof(VbeModeInfoBlock));
            return true;
        }
    }

    // 2. Fallback: If it's the mode the bootloader set, use BootInfo
    auto current_fb = boot::BootInfo::the().get_framebuffer_info();
    if (current_fb.addr != 0) {
        // We only return this if we don't have a better source
        info->width = current_fb.width;
        info->height = current_fb.height;
        info->bpp = current_fb.bpp;
        info->pitch = current_fb.pitch;
        info->framebuffer = static_cast<uint32_t>(current_fb.addr);
        info->attributes = (1 << 7) | (1 << 0); // LFB + Mode supported
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
