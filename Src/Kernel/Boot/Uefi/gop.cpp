#include <Kernel/Boot/Uefi/gop.h>
#include <Kernel/Boot/Uefi/uefi_types.h>
#include <LibFK/Core/Assertions.h>

namespace uefi {

// Graphics Output Protocol GUID
static const EFI_GUID EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID = {
  0x9042a9de, 0x23dc, 0x4a38,
  {0x96, 0xfb, 0x7a, 0xde, 0xd0, 0x80, 0x51, 0x6a}
};

boot::FramebufferInfo initialize_gop(EFI_SYSTEM_TABLE *system_table) {
  boot::FramebufferInfo fb_info{};
  
  if (!system_table || !system_table->BootServices) {
    return fb_info;
  }

  // Locate Graphics Output Protocol
  EFI_GRAPHICS_OUTPUT_PROTOCOL *gop = nullptr;
  EFI_STATUS status = (*system_table->BootServices->LocateProtocol)(
      &EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID,
      nullptr,
      reinterpret_cast<void **>(&gop));

  if (status != EFI_SUCCESS) {
    // GOP not available - this is OK, we'll fall back to VGA text mode
    return fb_info;
  }

  if (!gop || !gop->Mode) {
    // GOP found but invalid - fall back to VGA text mode
    return fb_info;
  }

  // Get current mode information
  auto *mode = gop->Mode;
  if (!mode->ModeInfo || !mode->FrameBufferBase) {
    return fb_info;
  }

  // Populate framebuffer info
  fb_info.addr = mode->FrameBufferBase;
  fb_info.width = mode->ModeInfo->HorizontalResolution;
  fb_info.height = mode->ModeInfo->VerticalResolution;
  
  // Calculate bits per pixel from pixel format
  // PixelFormat: 0=RGB, 1=BGR, 2=BitMask, 3=BltOnly, 4=FormatMax
  uint32_t pixel_format = mode->ModeInfo->PixelFormat;
  if (pixel_format == 0 || pixel_format == 1) {
    // RGB or BGR - calculate bpp from masks
    uint32_t total_mask = mode->ModeInfo->PixelInformation.RedMask |
                          mode->ModeInfo->PixelInformation.GreenMask |
                          mode->ModeInfo->PixelInformation.BlueMask |
                          mode->ModeInfo->PixelInformation.ReservedMask;
    // Count bits in mask
    fb_info.bpp = 0;
    while (total_mask) {
      fb_info.bpp++;
      total_mask >>= 1;
    }
    // Calculate pitch (bytes per scanline)
    fb_info.pitch = mode->ModeInfo->PixelsPerScanLine * (fb_info.bpp / 8);
  } else {
    // BitMask or other format - default to 32-bit
    fb_info.bpp = 32;
    fb_info.pitch = mode->ModeInfo->PixelsPerScanLine * 4;
  }
  
  // Extract pixel format information
  auto &pixel_info = mode->ModeInfo->PixelInformation;
  fb_info.red_mask = pixel_info.RedMask;
  fb_info.green_mask = pixel_info.GreenMask;
  fb_info.blue_mask = pixel_info.BlueMask;
  
  // Calculate bit positions (simplified - assumes standard RGB)
  // This is a simplified calculation; real implementation should handle all formats
  if (pixel_info.RedMask == 0xFF0000 && 
      pixel_info.GreenMask == 0xFF00 && 
      pixel_info.BlueMask == 0xFF) {
    fb_info.red_pos = 16;
    fb_info.green_pos = 8;
    fb_info.blue_pos = 0;
  } else {
    // Default to standard RGB
    fb_info.red_pos = 16;
    fb_info.green_pos = 8;
    fb_info.blue_pos = 0;
  }

  return fb_info;
}

} // namespace uefi
