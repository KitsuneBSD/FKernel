#include <Kernel/Boot/boot_info.h>
#include <Kernel/Boot/multiboot2.h>
#include <Kernel/Boot/multiboot_interpreter.h>
#include <LibFK/Core/Assertions.h>
#include <LibFK/Algorithms/log.h>

namespace boot {

BootInfo &BootInfo::the() {
  static BootInfo instance;
  return instance;
}

void BootInfo::detect_boot_mode(void *mb_ptr) {
  assert(mb_ptr && "detect_boot_mode: Multiboot pointer is null!");

  multiboot2::MultibootParser parser(mb_ptr);

  // Check for 64-bit EFI boot
  auto efi64_tag = parser.find_tag<multiboot2::TagEFI64>(multiboot2::TagType::EFI64);
  if (efi64_tag) {
    m_boot_mode = BootMode::EFI64;
    m_efi_system_table = reinterpret_cast<void *>(efi64_tag->efi_system_table);
    fk::algorithms::klog("BOOT", "Detected 64-bit EFI boot");
    fk::algorithms::klog("BOOT", "  EFI System Table: %p", m_efi_system_table);
  }
  // Check for 32-bit EFI boot
  else if (auto efi32_tag = parser.find_tag<multiboot2::TagEFI32>(multiboot2::TagType::EFI32)) {
    m_boot_mode = BootMode::EFI32;
    m_efi_system_table = reinterpret_cast<void *>(efi32_tag->efi_system_table);
    fk::algorithms::klog("BOOT", "Detected 32-bit EFI boot");
    fk::algorithms::klog("BOOT", "  EFI System Table: %p", m_efi_system_table);
  }
  // Default to BIOS boot
  else {
    m_boot_mode = BootMode::BIOS;
    fk::algorithms::klog("BOOT", "Detected legacy BIOS boot (Multiboot2)");
  }

  // Check for EFI 64-bit image handle
  auto efi64_image_tag = parser.find_tag<multiboot2::TagEFI64ImageHandle>(multiboot2::TagType::EFI64ImageHandle);
  if (efi64_image_tag) {
    m_efi_image_handle = reinterpret_cast<void *>(efi64_image_tag->efi_image_handle);
    fk::algorithms::klog("BOOT", "  EFI Image Handle: %p", m_efi_image_handle);
  }
  // Check for EFI 32-bit image handle
  else if (auto efi32_image_tag = parser.find_tag<multiboot2::TagEFI32ImageHandle>(multiboot2::TagType::EFI32ImageHandle)) {
    m_efi_image_handle = reinterpret_cast<void *>(efi32_image_tag->efi_image_handle);
    fk::algorithms::klog("BOOT", "  EFI Image Handle: %p", m_efi_image_handle);
  }

  // Check for EFI boot services availability
  auto efi_boot_services_tag = parser.find_tag<multiboot2::TagEFIBootServices>(multiboot2::TagType::EFIBootServices);
  if (efi_boot_services_tag) {
    m_efi_boot_services_available = true;
    fk::algorithms::klog("BOOT", "  EFI Boot Services available");
  }

  // Check for framebuffer info
  auto fb_tag = parser.find_tag<multiboot2::TagFramebuffer>(multiboot2::TagType::Framebuffer);
  if (fb_tag) {
    m_has_framebuffer = true;
    m_framebuffer_info.addr = fb_tag->framebuffer_addr;
    m_framebuffer_info.pitch = fb_tag->framebuffer_pitch;
    m_framebuffer_info.width = fb_tag->framebuffer_width;
    m_framebuffer_info.height = fb_tag->framebuffer_height;
    m_framebuffer_info.bpp = fb_tag->framebuffer_bpp;
    m_framebuffer_info.type = fb_tag->framebuffer_type;
    m_framebuffer_info.rgb = fb_tag->rgb;

    fk::algorithms::klog("BOOT", "  Framebuffer: addr=%p pitch=%u %ux%u bpp=%u type=%u",
                         reinterpret_cast<void *>(m_framebuffer_info.addr), m_framebuffer_info.pitch,
                         m_framebuffer_info.width, m_framebuffer_info.height, m_framebuffer_info.bpp,
                         m_framebuffer_info.type);
  }
}

} // namespace boot
