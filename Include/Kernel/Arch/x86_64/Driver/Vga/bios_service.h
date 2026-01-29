#pragma once

#include <LibFK/Types/types.h>
#include <Kernel/Arch/x86_64/Driver/Vga/vbe_types.h>

namespace fkernel::drivers::vesa {

extern "C" {
    /**
     * @brief Call BIOS INT 10h, AX=4F00h (Get Controller Info)
     * @param info Pointer to VbeInfoBlock
     * @return True on success
     */
    bool bios_int10h_vesa_get_controller_info(VbeInfoBlock* info);

    /**
     * @brief Call BIOS INT 10h, AX=4F01h (Get Mode Info)
     * @param mode Video mode number
     * @param info Pointer to VbeModeInfoBlock
     * @return True on success
     */
    bool bios_int10h_vesa_get_mode_info(uint16_t mode, VbeModeInfoBlock* info);

    /**
     * @brief Call BIOS INT 10h, AX=4F02h (Set Video Mode)
     * @param mode Video mode number
     * @return True on success
     */
    bool bios_int10h_vesa_set_mode(uint16_t mode);
}

} // namespace fkernel::drivers::vesa
