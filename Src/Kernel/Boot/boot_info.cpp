#include <Kernel/Boot/Multiboot/multiboot2.h>
#include <Kernel/Boot/Multiboot/multiboot_interpreter.h>
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
  fk::algorithms::klog("BOOT", "Detected legacy BIOS boot (Multiboot2)");

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
  }

  if (m_memory_map_iterator) {
    fk::algorithms::klog("BOOT", "Memory map iterator created successfully.");
  } else {
    fk::algorithms::kwarn("BOOT",
                          "No memory map data available to create iterator!");
  }
}

} // namespace boot
