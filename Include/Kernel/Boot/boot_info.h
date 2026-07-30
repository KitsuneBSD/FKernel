#pragma once

#include <LibFK/Types/types.h>
#include <LibFK/Container/vector.h>
#include <LibFK/Text/fixed_string.h>

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
 * Provides unified interface for Multiboot2 boot
 */
class BootInfo {
private:
  BootMode m_boot_mode{BootMode::Unknown};
  
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

  // Kernel cmdline and parsed boot parameters
  fk::text::fixed_string<512> m_kernel_cmdline{};
  fk::text::fixed_string<256> m_init_path{"/sbin/init"};
  fk::text::fixed_string<256> m_root_device{};
  fk::text::fixed_string<64>  m_rootfstype{};
  bool m_quiet{false};

  // Internal initialization flags
  bool m_initialized{false};

  void parse_cmdline(const char* cmdline);

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
   * @brief Creates iterators for memory map after heap is initialized
   */
  void create_iterators();

  /**
   * @brief Get the current boot mode
   */
  BootMode get_boot_mode() const { return m_boot_mode; }

  /**
   * @brief Check if booting in Multiboot2 mode
   */
  bool is_multiboot2_boot() const { return m_boot_mode == BootMode::Multiboot2; }

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
   * @brief Kernel cmdline access
   */
  const char* get_kernel_cmdline() const { return m_kernel_cmdline.c_str(); }
  const char* get_init_path()      const { return m_init_path.c_str(); }
  const char* get_root_device()    const { return m_root_device.c_str(); }
  const char* get_rootfstype()     const { return m_rootfstype.c_str(); }
  bool        is_quiet()           const { return m_quiet; }

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
