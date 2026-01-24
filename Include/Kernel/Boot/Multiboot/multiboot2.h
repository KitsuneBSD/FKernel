#pragma once

#include <LibFK/Types/types.h>

namespace multiboot2 {

/**
 * @brief Multiboot2 header alignment
 */
constexpr uint32_t HEADER_ALIGN = 8;

/**
 * @brief Multiboot2 header magic number
 */
constexpr uint32_t HEADER_MAGIC = 0xe85250d6;

/**
 * @brief Magic number passed by the bootloader
 */
constexpr uint32_t BOOTLOADER_MAGIC = 0x36d76289;

/**
 * @brief Module alignment
 */
constexpr uint32_t MOD_ALIGN = 0x00001000;

/**
 * @brief Info structure alignment
 */
constexpr uint32_t INFO_ALIGN = 0x00000008;

/**
 * @brief Tag alignment
 */
constexpr uint32_t TAG_ALIGN = 8;

/**
 * @brief Multiboot2 memory types
 */
enum class MemoryType : uint32_t {
  Available = 1,             ///< Usable by the OS
  Reserved = 2,              ///< Reserved, must not be used
  ACPIReclaimable = 3,       ///< ACPI reclaimable memory
  ACPINVS = 4,               ///< ACPI NVS (non-volatile)
  BadMemory = 5,             ///< Defective memory regions
  BootloaderReclaimable = 6, ///< Can be reused after boot
  KernelAndModules = 7,      ///< Occupied by kernel/modules (non-standard)
  Unknown = 0xFFFFFFFF       ///< Unrecognized type
};

/**
 * @brief Multiboot2 tag types
 */
enum class TagType : uint32_t {
  End = 0,
  Cmdline = 1,
  BootLoaderName = 2,
  Module = 3,
  BasicMemInfo = 4,
  BootDev = 5,
  MMap = 6,
  VBE = 7,
  Framebuffer = 8,
  ELFSections = 9,
  APM = 10,
  EFI32 = 11,
  EFI64 = 12,
  SMBIOS = 13,
  ACPIOld = 14,
  ACPINew = 15,
  Network = 16,
  EFIMMap = 17,
  EFIBootServices = 18,
  EFI32ImageHandle = 19,
  EFI64ImageHandle = 20,
  LoadBaseAddr = 21
};

/**
 * @brief Multiboot2 header tag types
 */
enum class HeaderTagType : uint32_t {
  End = 0,
  InformationRequest = 1,
  Address = 2,
  EntryAddress = 3,
  ConsoleFlags = 4,
  Framebuffer = 5,
  ModuleAlign = 6,
  EFIBootServices = 7,
  EFI32EntryAddress = 8,
  EFI64EntryAddress = 9,
  Relocatable = 10
};

/**
 * @brief Multiboot2 console flags
 */
enum class ConsoleFlags : uint32_t {
  ConsoleRequired = 1,
  EGATextSupported = 2
};

/**
 * @brief Supported architectures
 */
enum class Architecture : uint32_t { I386 = 0, MIPS32 = 4 };

/**
 * @brief Load preferences for multiboot modules
 */
enum class LoadPreference : uint32_t { None = 0, Low = 1, High = 2 };

/**
 * @brief Generic multiboot2 tag
 */
struct alignas(TAG_ALIGN) Tag {
  TagType type;  ///< Tag type
  uint32_t size; ///< Size of the tag including header

  /**
   * @brief Get the next tag in memory
   * @return Pointer to the next tag
   */
  Tag const *next() const noexcept {
    return reinterpret_cast<Tag const *>(
        reinterpret_cast<uint8_t const *>(this) + ((size + 7) & ~7u));
  }
};
static_assert(sizeof(Tag) == 8, "Tag struct must be exactly 8 bytes");
static_assert(alignof(Tag) == 8, "Tag struct must be aligned to 8 bytes");

/**
 * @brief Tag containing a string
 */
struct alignas(TAG_ALIGN) TagString {
  Tag tag;
  char string[0]; ///< Flexible array member

  [[nodiscard]]
  char const *get_string() const noexcept {
    return reinterpret_cast<char const *>(this + 1);
  }
};
static_assert(alignof(TagString) == 8, "TagString must be aligned to 8 bytes");

/**
 * @brief Tag describing a loaded module
 */
struct alignas(TAG_ALIGN) TagModule {
  Tag tag;
  uint32_t mod_start; ///< Start address of the module
  uint32_t mod_end;   ///< End address of the module
  char cmdline[0];    ///< Module command line (flexible array member)

  [[nodiscard]]
  char const *get_cmdline() const noexcept {
    return reinterpret_cast<char const *>(this + 1);
  }
};
static_assert(alignof(TagModule) == 8, "TagModule must be aligned to 8 bytes");

/**
 * @brief Multiboot2 information structure
 */
struct alignas(TAG_ALIGN) Info {
  uint32_t total_size; ///< Total size of the info structure
  uint32_t reserved;   ///< Reserved field
  Tag tags[0];         ///< Flexible array of tags

  [[nodiscard]]
  Tag const *first_tag() const noexcept {
    return tags;
  }
};
static_assert(sizeof(Info) == 8, "Info struct must be exactly 8 bytes");
static_assert(alignof(Info) == 8, "Info struct must be aligned to 8 bytes");

/**
 * @brief Memory map tag
 */
struct alignas(TAG_ALIGN) TagMemoryMap : Tag {
  struct Entry {
    uint64_t base_addr; ///< Base address of memory region
    uint64_t length;    ///< Length of memory region
    uint32_t type;      ///< Memory type (MemoryType enum)
    uint32_t reserved;  ///< Reserved field
  };
  static_assert(sizeof(Entry) == 24,
                "TagMemoryMap::Entry must be exactly 24 bytes");
  static_assert(alignof(Entry) == 8,
                "TagMemoryMap::Entry must be aligned to 8 bytes");

  uint32_t entry_size;    ///< Size of each entry
  uint32_t entry_version; ///< Version of the entry format
  Entry entries[0];       ///< Flexible array of entries

  Entry const *begin() const noexcept { return entries; }

  Entry const *end() const noexcept {
    size_t entries_size = size - sizeof(TagMemoryMap);
    size_t count = entries_size / entry_size;
    return entries + count;
  }
};
static_assert(alignof(TagMemoryMap) == 8,
              "TagMemoryMap must be aligned to 8 bytes");

/**
 * @brief Framebuffer tag
 */
struct alignas(TAG_ALIGN) TagFramebuffer : Tag {
  uint64_t framebuffer_addr;   ///< Physical address of framebuffer
  uint32_t framebuffer_pitch;  ///< Number of bytes per row
  uint32_t framebuffer_width;  ///< Width in pixels
  uint32_t framebuffer_height; ///< Height in pixels
  uint8_t framebuffer_bpp;     ///< Bits per pixel
  uint8_t framebuffer_type;    ///< 0=indexed, 1=RGB, 2=EGA text
  uint16_t reserved;

  struct RGBInfo {
    uint8_t red_field_position;
    uint8_t red_mask_size;
    uint8_t green_field_position;
    uint8_t green_mask_size;
    uint8_t blue_field_position;
    uint8_t blue_mask_size;
  } rgb;
};
static_assert(alignof(TagFramebuffer) == 8,
              "TagFramebuffer must be aligned to 8 bytes");

/**
 * @brief EFI 32-bit system table tag
 */
struct alignas(TAG_ALIGN) TagEFI32 : Tag {
  uint32_t efi_system_table; ///< Physical address of EFI system table (32-bit)
};
static_assert(alignof(TagEFI32) == 8, "TagEFI32 must be aligned to 8 bytes");

/**
 * @brief EFI 64-bit system table tag
 */
struct alignas(TAG_ALIGN) TagEFI64 : Tag {
  uint64_t efi_system_table; ///< Physical address of EFI system table (64-bit)
};
static_assert(alignof(TagEFI64) == 8, "TagEFI64 must be aligned to 8 bytes");

/**
 * @brief EFI 32-bit image handle tag
 */
struct alignas(TAG_ALIGN) TagEFI32ImageHandle : Tag {
  uint32_t efi_image_handle; ///< EFI image handle (32-bit)
};
static_assert(alignof(TagEFI32ImageHandle) == 8,
              "TagEFI32ImageHandle must be aligned to 8 bytes");

/**
 * @brief EFI 64-bit image handle tag
 */
struct alignas(TAG_ALIGN) TagEFI64ImageHandle : Tag {
  uint64_t efi_image_handle; ///< EFI image handle (64-bit)
};
static_assert(alignof(TagEFI64ImageHandle) == 8,
              "TagEFI64ImageHandle must be aligned to 8 bytes");

/**
 * @brief EFI boot services availability tag
 */
struct alignas(TAG_ALIGN) TagEFIBootServices : Tag {
  // This tag only indicates that boot services are available
  // No additional data fields
};
static_assert(alignof(TagEFIBootServices) == 8,
              "TagEFIBootServices must be aligned to 8 bytes");

/**
 * @brief EFI memory map tag
 */
struct alignas(TAG_ALIGN) TagEFIMMap : Tag {
  uint32_t descriptor_size;     ///< Size of each EFI memory descriptor
  uint32_t descriptor_version;  ///< Version of the descriptor format
  uint8_t descriptors[0];       ///< Flexible array of EFI memory descriptors

  /**
   * @brief EFI memory descriptor
   */
  struct Descriptor {
    uint32_t type;        ///< EFI memory type
    uint32_t padding;     ///< Padding for alignment
    uint64_t physical_start; ///< Physical start address
    uint64_t virtual_start;  ///< Virtual start address
    uint64_t number_of_pages; ///< Number of 4KB pages
    uint64_t attribute;       ///< Memory attributes
  };
};
static_assert(alignof(TagEFIMMap) == 8, "TagEFIMMap must be aligned to 8 bytes");

} // namespace multiboot2
