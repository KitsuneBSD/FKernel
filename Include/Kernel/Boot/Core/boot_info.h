#pragma once

#include <LibFK/Types/types.h>
#include <LibFK/Container/Sequence/vector.h>
#include <LibFK/Text/fixed_string.h>

#include <Kernel/Boot/Core/boot_mode.h>
#include <Kernel/Boot/Info/acpi_table_info.h>
#include <Kernel/Boot/Info/boot_framebuffer_info.h>
#include <Kernel/Boot/Info/memory_map_entry.h>
#include <Kernel/Boot/Info/memory_map_iterator.h>
#include <Kernel/Boot/Info/module_info.h>

namespace boot {

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
