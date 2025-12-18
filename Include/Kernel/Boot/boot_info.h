#pragma once

#include <LibFK/Types/types.h>
#include <Kernel/Boot/multiboot2.h>

namespace boot {

/**
 * @brief Boot mode detection
 */
enum class BootMode : uint32_t {
  Unknown = 0,
  BIOS = 1,      ///< Legacy BIOS boot (via Multiboot2)
  EFI32 = 2,     ///< 32-bit EFI boot (via Multiboot2)
  EFI64 = 3,     ///< 64-bit EFI boot (via Multiboot2)
};

/**
 * @brief Boot information structure
 * Provides unified interface for both BIOS and EFI boots
 */
class BootInfo {
private:
  BootMode m_boot_mode{BootMode::Unknown};
  void *m_efi_system_table{nullptr};
  void *m_efi_image_handle{nullptr};
  bool m_efi_boot_services_available{false};
  // Framebuffer info (populated when Multiboot2 framebuffer tag is present)
  bool m_has_framebuffer{false};
  struct FramebufferInfo {
    uint64_t addr{0};
    uint32_t pitch{0};
    uint32_t width{0};
    uint32_t height{0};
    uint8_t bpp{0};
    uint8_t type{0};
    multiboot2::TagFramebuffer::RGBInfo rgb{};
  } m_framebuffer_info{};

public:
  /**
   * @brief Detect boot mode from Multiboot2 information
   * @param mb_ptr Pointer to Multiboot2 info structure
   */
  void detect_boot_mode(void *mb_ptr);

  /**
   * @brief Get the current boot mode
   */
  BootMode get_boot_mode() const { return m_boot_mode; }

  /**
   * @brief Check if booting in EFI mode
   */
  bool is_efi_boot() const {
    return m_boot_mode == BootMode::EFI32 || m_boot_mode == BootMode::EFI64;
  }

  /**
   * @brief Check if booting in BIOS mode
   */
  bool is_bios_boot() const { return m_boot_mode == BootMode::BIOS; }

  /**
   * @brief Check if booting in 64-bit EFI mode
   */
  bool is_efi64() const { return m_boot_mode == BootMode::EFI64; }

  /**
   * @brief Check if booting in 32-bit EFI mode
   */
  bool is_efi32() const { return m_boot_mode == BootMode::EFI32; }

  /**
   * @brief Get EFI system table pointer
   */
  void *get_efi_system_table() const { return m_efi_system_table; }

  /**
   * @brief Get EFI image handle
   */
  void *get_efi_image_handle() const { return m_efi_image_handle; }

  /**
   * @brief Check if EFI boot services are available
   */
  bool are_efi_boot_services_available() const {
    return m_efi_boot_services_available;
  }

  /**
   * @brief Framebuffer helpers
   */
  bool has_framebuffer() const { return m_has_framebuffer; }
  FramebufferInfo get_framebuffer_info() const { return m_framebuffer_info; }

  /**
   * @brief Singleton access
   */
  static BootInfo &the();
};

} // namespace boot
