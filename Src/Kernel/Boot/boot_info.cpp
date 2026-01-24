#include <Kernel/Boot/Multiboot/multiboot2.h>
#include <Kernel/Boot/Multiboot/multiboot_interpreter.h>
#include <Kernel/Boot/Uefi/uefi_loader.h>
#include <Kernel/Boot/boot_info.h>
#include <LibFK/Algorithms/log.h>
#include <LibFK/Core/Assertions.h>

namespace boot {

/**
 * @brief Multiboot2 memory map iterator adapter
 */
class Multiboot2MemoryMapIterator : public MemoryMapIterator {
private:
  const multiboot2::TagMemoryMap *m_mmap;
  multiboot2::TagMemoryMap::Entry const *m_current;
  multiboot2::TagMemoryMap::Entry const *m_end;

public:
  explicit Multiboot2MemoryMapIterator(const multiboot2::TagMemoryMap *mmap)
      : m_mmap(mmap) {
    if (m_mmap) {
      m_current = m_mmap->begin();
      m_end = m_mmap->end();
    } else {
      m_current = nullptr;
      m_end = nullptr;
    }
  }

  bool has_next() const override { return m_current && m_current < m_end; }

  MemoryMapEntry next() override {
    assert(has_next() && "Multiboot2MemoryMapIterator: No more entries!");

    MemoryMapEntry entry;
    entry.base_addr = m_current->base_addr;
    entry.length = m_current->length;
    entry.type = m_current->type;
    entry.is_available = multiboot2::is_available(m_current->type);

    m_current++;
    return entry;
  }

  void reset() override {
    if (m_mmap) {
      m_current = m_mmap->begin();
      m_end = m_mmap->end();
    } else {
      m_current = nullptr;
      m_end = nullptr;
    }
  }
};

BootInfo &BootInfo::the() {
  static BootInfo instance;
  return instance;
}

BootInfo::~BootInfo() {
  delete m_memory_map_iterator;
  m_memory_map_iterator = nullptr;
}

void BootInfo::initialize_from_multiboot2(void *mb_ptr) {
  assert(mb_ptr && "initialize_from_multiboot2: Multiboot pointer is null!");
  assert(!m_initialized && "BootInfo already initialized!");

  m_raw_mb_ptr = mb_ptr;
  multiboot2::MultibootParser parser(mb_ptr);

  m_boot_mode = BootMode::Multiboot2;

  // Check for 64-bit EFI boot (via Multiboot2)
  auto efi64_tag =
      parser.find_tag<multiboot2::TagEFI64>(multiboot2::TagType::EFI64);
  if (efi64_tag) {
    m_efi_system_table = reinterpret_cast<void *>(efi64_tag->efi_system_table);
    fk::algorithms::klog("BOOT", "Detected 64-bit EFI boot (via Multiboot2)");
    fk::algorithms::klog("BOOT", "  EFI System Table: %p", m_efi_system_table);
  }
  // Check for 32-bit EFI boot (via Multiboot2)
  else if (auto efi32_tag = parser.find_tag<multiboot2::TagEFI32>(
               multiboot2::TagType::EFI32)) {
    m_efi_system_table = reinterpret_cast<void *>(efi32_tag->efi_system_table);
    fk::algorithms::klog("BOOT", "Detected 32-bit EFI boot (via Multiboot2)");
    fk::algorithms::klog("BOOT", "  EFI System Table: %p", m_efi_system_table);
  } else {
    fk::algorithms::klog("BOOT", "Detected legacy BIOS boot (Multiboot2)");
  }

  // Check for EFI 64-bit image handle
  auto efi64_image_tag = parser.find_tag<multiboot2::TagEFI64ImageHandle>(
      multiboot2::TagType::EFI64ImageHandle);
  if (efi64_image_tag) {
    m_efi_image_handle =
        reinterpret_cast<void *>(efi64_image_tag->efi_image_handle);
    fk::algorithms::klog("BOOT", "  EFI Image Handle: %p", m_efi_image_handle);
  }
  // Check for EFI 32-bit image handle
  else if (auto efi32_image_tag =
               parser.find_tag<multiboot2::TagEFI32ImageHandle>(
                   multiboot2::TagType::EFI32ImageHandle)) {
    m_efi_image_handle =
        reinterpret_cast<void *>(efi32_image_tag->efi_image_handle);
    fk::algorithms::klog("BOOT", "  EFI Image Handle: %p", m_efi_image_handle);
  }

  // Check for EFI boot services availability
  auto efi_boot_services_tag = parser.find_tag<multiboot2::TagEFIBootServices>(
      multiboot2::TagType::EFIBootServices);
  if (efi_boot_services_tag) {
    m_efi_boot_services_available = true;
    fk::algorithms::klog("BOOT", "  EFI Boot Services available");
  }

  // Get memory map
  auto mmap_tag =
      parser.find_tag<multiboot2::TagMemoryMap>(multiboot2::TagType::MMap);
  if (mmap_tag) {
    m_raw_mmap_ptr = (void *)mmap_tag;
    fk::algorithms::klog("BOOT",
                         "  Memory map available (deferred iterator creation)");
  }

  // Check for framebuffer info
  // Note: framebuffer_type: 0=indexed, 1=RGB, 2=EGA text (we only accept 0 and
  // 1)
  auto fb_tag = parser.find_tag<multiboot2::TagFramebuffer>(
      multiboot2::TagType::Framebuffer);
  if (fb_tag && fb_tag->framebuffer_type != 2) {
    // Only accept indexed (0) or RGB (1) framebuffers, not EGA text (2)
    m_has_framebuffer = true;
    m_framebuffer_info.addr = fb_tag->framebuffer_addr;
    m_framebuffer_info.pitch = fb_tag->framebuffer_pitch;
    m_framebuffer_info.width = fb_tag->framebuffer_width;
    m_framebuffer_info.height = fb_tag->framebuffer_height;
    m_framebuffer_info.bpp = fb_tag->framebuffer_bpp;
    m_framebuffer_info.red_pos = fb_tag->rgb.red_field_position;
    m_framebuffer_info.red_mask = fb_tag->rgb.red_mask_size;
    m_framebuffer_info.green_pos = fb_tag->rgb.green_field_position;
    m_framebuffer_info.green_mask = fb_tag->rgb.green_mask_size;
    m_framebuffer_info.blue_pos = fb_tag->rgb.blue_field_position;
    m_framebuffer_info.blue_mask = fb_tag->rgb.blue_mask_size;

    fk::algorithms::klog("BOOT",
                         "  Framebuffer: addr=%p pitch=%u %ux%u bpp=%u type=%u",
                         reinterpret_cast<void *>(m_framebuffer_info.addr),
                         m_framebuffer_info.pitch, m_framebuffer_info.width,
                         m_framebuffer_info.height, m_framebuffer_info.bpp,
                         fb_tag->framebuffer_type);
  } else if (fb_tag && fb_tag->framebuffer_type == 2) {
    fk::algorithms::klog("BOOT", "  Framebuffer tag found but type is EGA text "
                                 "(2), ignoring - will use VGA text mode");
  }

  // Get ACPI tables (if available via Multiboot2)
  // Note: ACPI tables are typically found via RSDP, which is searched in memory
  // For now, we'll leave ACPI info empty and let ACPIManager find RSDP itself

  m_initialized = true;
  fk::algorithms::klog("BOOT", "BootInfo initialized from Multiboot2");
}

void BootInfo::initialize_from_uefi(void *efi_system_table,
                                    void *efi_image_handle,
                                    const FramebufferInfo &framebuffer_info,
                                    void *memory_map, size_t descriptor_count,
                                    size_t descriptor_size,
                                    const AcpiTableInfo &acpi_info) {
  assert(!m_initialized && "BootInfo already initialized!");

  m_boot_mode = BootMode::Uefi;
  m_efi_system_table = efi_system_table;
  m_efi_image_handle = efi_image_handle;
  m_efi_boot_services_available = false; // Set to false after ExitBootServices
  m_has_framebuffer = framebuffer_info.addr != 0;
  m_framebuffer_info = framebuffer_info;

  m_raw_mmap_ptr = memory_map;
  m_raw_mmap_count = descriptor_count;
  m_raw_mmap_desc_size = descriptor_size;

  m_acpi_info = acpi_info;

  m_initialized = true;
  fk::algorithms::klog("BOOT", "BootInfo initialized from UEFI");
  fk::algorithms::klog("BOOT", "  EFI System Table: %p", m_efi_system_table);
  fk::algorithms::klog("BOOT", "  EFI Image Handle: %p", m_efi_image_handle);
  if (m_has_framebuffer) {
    fk::algorithms::klog("BOOT", "  Framebuffer: addr=%p pitch=%u %ux%u bpp=%u",
                         reinterpret_cast<void *>(m_framebuffer_info.addr),
                         m_framebuffer_info.pitch, m_framebuffer_info.width,
                         m_framebuffer_info.height, m_framebuffer_info.bpp);
  }
}

void BootInfo::create_iterators() {
  assert(m_initialized);
  if (m_memory_map_iterator)
    return;

  if (m_boot_mode == BootMode::Multiboot2) {
    if (m_raw_mmap_ptr) {
      m_memory_map_iterator = new Multiboot2MemoryMapIterator(
          (const multiboot2::TagMemoryMap *)m_raw_mmap_ptr);
    }

    if (m_raw_mb_ptr) {
      multiboot2::MultibootParser parser(m_raw_mb_ptr);
      for (auto const *tag = parser.first_tag();
           tag && tag->type != multiboot2::TagType::End; tag = tag->next()) {
        if (tag->type == multiboot2::TagType::Module) {
          auto mod = reinterpret_cast<multiboot2::TagModule const *>(tag);
          m_modules.push_back({.start = mod->mod_start,
                               .end = mod->mod_end,
                               .cmdline = mod->get_cmdline()});
          fk::algorithms::klog("BOOT", "Found module: %s (%p - %p)",
                               mod->get_cmdline(),
                               (void *)(uintptr_t)mod->mod_start,
                               (void *)(uintptr_t)mod->mod_end);
        }
      }
    }
  } else if (m_boot_mode == BootMode::Uefi) {
    if (m_raw_mmap_ptr) {
      m_memory_map_iterator = new uefi::UefiMemoryMapIterator(
          (uefi::EFI_MEMORY_DESCRIPTOR *)m_raw_mmap_ptr, m_raw_mmap_count,
          m_raw_mmap_desc_size);
    }
  }

  if (m_memory_map_iterator) {
    fk::algorithms::klog("BOOT", "Memory map iterator created successfully.");
  } else {
    fk::algorithms::kwarn("BOOT",
                          "No memory map data available to create iterator!");
  }
}

} // namespace boot
