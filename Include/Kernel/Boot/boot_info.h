#pragma once

#include <LibFK/Types/types.h>
#include <LibFK/Container/vector.h>

namespace boot {

/**
 * @brief Boot module information
 */
struct ModuleInfo {
  uint64_t start;
  uint64_t end;
  const char* cmdline;
};

/**
 * @brief Boot mode detection
 */
enum class BootMode : uint32_t {
  Unknown = 0,
  Multiboot2 = 1,  ///< Legacy BIOS boot (via Multiboot2)
  Uefi = 2,        ///< Native UEFI boot
};

/**
 * @brief Unified memory map entry
 * Abstracts differences between Multiboot2 and EFI memory maps
 */
struct MemoryMapEntry {
  uint64_t base_addr;  ///< Base address of memory region
  uint64_t length;     ///< Length of memory region
  uint32_t type;        ///< Memory type (unified)
  bool is_available;   ///< Whether this memory is available for use
};

/**
 * @brief Unified framebuffer information
 * Works with both GOP and Multiboot2 framebuffer
 */
struct FramebufferInfo {
  uint64_t addr{0};      ///< Physical address of framebuffer
  uint32_t pitch{0};     ///< Bytes per row
  uint32_t width{0};     ///< Width in pixels
  uint32_t height{0};    ///< Height in pixels
  uint8_t bpp{0};        ///< Bits per pixel
  uint8_t red_pos{0};    ///< Red field position
  uint8_t red_mask{0};   ///< Red mask size
  uint8_t green_pos{0};  ///< Green field position
  uint8_t green_mask{0}; ///< Green mask size
  uint8_t blue_pos{0};   ///< Blue field position
  uint8_t blue_mask{0};  ///< Blue mask size
};

/**
 * @brief ACPI table information
 */
struct AcpiTableInfo {
  void *rsdp{nullptr};  ///< RSDP pointer (if available)
  void *rsdt{nullptr};  ///< RSDT pointer (if available)
  void *xsdt{nullptr};  ///< XSDT pointer (if available)
};

/**
 * @brief Memory map iterator interface
 */
class MemoryMapIterator {
public:
  virtual ~MemoryMapIterator() = default;
  virtual bool has_next() const = 0;
  virtual MemoryMapEntry next() = 0;
  virtual void reset() = 0;
};

/**
 * @brief Boot information structure
 * Provides unified interface for both Multiboot2 and UEFI boots
 */
class BootInfo {
private:
  BootMode m_boot_mode{BootMode::Unknown};
  void *m_efi_system_table{nullptr};
  void *m_efi_image_handle{nullptr};
  bool m_efi_boot_services_available{false};
  
  // Unified framebuffer info
  bool m_has_framebuffer{false};
  FramebufferInfo m_framebuffer_info{};
  
  // Unified ACPI info
  AcpiTableInfo m_acpi_info{};
  
  // Memory map iterator (owned by adapter)
  MemoryMapIterator *m_memory_map_iterator{nullptr};

  // Modules list
  fk::containers::Vector<ModuleInfo> m_modules;
  
  // Raw boot data for late iterator creation
  void* m_raw_mb_ptr{nullptr};
  void* m_raw_mmap_ptr{nullptr};
  size_t m_raw_mmap_count{0};
  size_t m_raw_mmap_desc_size{0};

  // Internal initialization flags
  bool m_initialized{false};

public:
  BootInfo() = default;
  ~BootInfo();
  
  // Disable copying
  BootInfo(const BootInfo &) = delete;
  BootInfo &operator=(const BootInfo &) = delete;

  /**
   * @brief Initialize from Multiboot2 information
   * @param mb_ptr Pointer to Multiboot2 info structure
   */
  void initialize_from_multiboot2(void *mb_ptr);

  /**
   * @brief Initialize from UEFI boot
   * @param efi_system_table EFI System Table pointer
   * @param efi_image_handle EFI Image Handle
   * @param framebuffer_info Framebuffer information from GOP
   * @param memory_map Raw EFI memory map descriptors
   * @param descriptor_count Number of descriptors in the map
   * @param descriptor_size Size of each descriptor
   * @param acpi_info ACPI table information
   */
  void initialize_from_uefi(void *efi_system_table,
                            void *efi_image_handle,
                            const FramebufferInfo &framebuffer_info,
                            void *memory_map,
                            size_t descriptor_count,
                            size_t descriptor_size,
                            const AcpiTableInfo &acpi_info);

  /**
   * @brief Creates iterators for memory map after heap is initialized
   */
  void create_iterators();

  /**
   * @brief Get the current boot mode
   */
  BootMode get_boot_mode() const { return m_boot_mode; }

  /**
   * @brief Check if booting in UEFI mode
   */
  bool is_uefi_boot() const { return m_boot_mode == BootMode::Uefi; }

  /**
   * @brief Check if booting in Multiboot2 mode
   */
  bool is_multiboot2_boot() const { return m_boot_mode == BootMode::Multiboot2; }

  /**
   * @brief Get EFI system table pointer (only valid for UEFI boot)
   */
  void *get_efi_system_table() const { return m_efi_system_table; }

  /**
   * @brief Get EFI image handle (only valid for UEFI boot)
   */
  void *get_efi_image_handle() const { return m_efi_image_handle; }

  /**
   * @brief Check if EFI boot services are available
   */
  bool are_efi_boot_services_available() const {
    return m_efi_boot_services_available;
  }

  /**
   * @brief Mark EFI boot services as no longer available (after ExitBootServices)
   */
  void set_efi_boot_services_unavailable() {
    m_efi_boot_services_available = false;
  }

  /**
   * @brief Framebuffer helpers
   */
  bool has_framebuffer() const { return m_has_framebuffer; }
  FramebufferInfo get_framebuffer_info() const { return m_framebuffer_info; }

  /**
   * @brief Memory map access
   */
  MemoryMapIterator *get_memory_map_iterator() const {
    return m_memory_map_iterator;
  }

  /**
   * @brief ACPI table access
   */
  AcpiTableInfo get_acpi_info() const { return m_acpi_info; }

  /**
   * @brief Module access
   */
  const fk::containers::Vector<ModuleInfo>& get_modules() const { return m_modules; }

  /**
   * @brief Check if BootInfo has been initialized
   */
  bool is_initialized() const { return m_initialized; }

  /**
   * @brief Get raw multiboot pointer
   */
  void* get_raw_multiboot_ptr() const { return m_raw_mb_ptr; }

  /**
   * @brief Singleton access
   */
  static BootInfo &the();
};

} // namespace boot
