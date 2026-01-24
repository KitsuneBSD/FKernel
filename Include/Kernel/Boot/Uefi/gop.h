#pragma once

#include <Kernel/Boot/Uefi/uefi_types.h>
#include <Kernel/Boot/boot_info.h>
#include <LibFK/Types/types.h>

namespace uefi {

/**
 * @brief Initialize Graphics Output Protocol and get framebuffer info
 * @param system_table EFI System Table
 * @return Framebuffer information, or empty if failed
 */
boot::FramebufferInfo initialize_gop(uefi::EFI_SYSTEM_TABLE *system_table);

} // namespace uefi
