#pragma once

#include <stddef.h>
#include <stdint.h>

namespace multiboot2 {

using u8 = uint8_t;
using u16 = uint16_t;
using u32 = uint32_t;
using u64 = uint64_t;

constexpr u32 HEADER_ALIGN = 8;
constexpr u32 HEADER_MAGIC = 0xe85250d6;
constexpr u32 BOOTLOADER_MAGIC = 0x36d76289;

constexpr u32 MOD_ALIGN = 0x00001000;
constexpr u32 INFO_ALIGN = 0x00000008;
constexpr u32 TAG_ALIGN = 8;

enum class TagType : u32 {
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

enum class HeaderTagType : u32 {
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

enum class ConsoleFlags : u32 { ConsoleRequired = 1, EGATextSupported = 2 };

enum class Architecture : u32 { I386 = 0, MIPS32 = 4 };

enum class LoadPreference : u32 { None = 0, Low = 1, High = 2 };

struct alignas(TAG_ALIGN) Tag {
  TagType type;
  u32 size;

  const Tag *next() const noexcept {
    return reinterpret_cast<const Tag *>(
        reinterpret_cast<const uint8_t *>(this) + ((size + 7) & ~7u));
  }
};

struct alignas(TAG_ALIGN) TagString {
  Tag tag;
  char string[0];

  [[nodiscard]]
  const char *get_string() const noexcept {
    return reinterpret_cast<const char *>(this + 1);
  }
};

struct alignas(TAG_ALIGN) TagModule {
  Tag tag;
  u32 mod_start;
  u32 mod_end;
  char cmdline[0];

  [[nodiscard]]
  const char *get_cmdline() const noexcept {
    return reinterpret_cast<const char *>(this + 1);
  }
};

struct alignas(TAG_ALIGN) Info {
  u32 total_size;
  u32 reserved;
  Tag tags[0];

  [[nodiscard]]
  const Tag *first_tag() const noexcept {
    return tags;
  }
};

struct alignas(TAG_ALIGN) TagMemoryMap : Tag {
  struct Entry {
    u64 base_addr;
    u64 length;
    u32 type;
    u32 reserved;
  };

  u32 entry_size;
  u32 entry_version;
  Entry entries[0];

  const Entry *begin() const noexcept { return entries; }

  const Entry *end() const noexcept {
    size_t entries_size = size - sizeof(TagMemoryMap);

    size_t count = entries_size / entry_size;

    return entries + count;
  }
};

static_assert(sizeof(Tag) == 8);
static_assert(alignof(Tag) == 8);
static_assert(alignof(Info) == 8);
}; // namespace multiboot2
