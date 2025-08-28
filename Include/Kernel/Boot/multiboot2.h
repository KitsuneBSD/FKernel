#pragma once

#include <LibC/stddef.h>
#include <LibFK/types.h>

namespace multiboot2 {
constexpr FK::dword HEADER_ALIGN = 8;
constexpr FK::dword HEADER_MAGIC = 0xe85250d6;
constexpr FK::dword BOOTLOADER_MAGIC = 0x36d76289;

constexpr FK::dword MOD_ALIGN = 0x00001000;
constexpr FK::dword INFO_ALIGN = 0x00000008;
constexpr FK::dword TAG_ALIGN = 8;

enum class MemoryType : FK::dword {
    Available = 1,             // Usável pelo sistema
    Reserved = 2,              // Reservado; não deve ser usado
    ACPIReclaimable = 3,       // Memória ACPI reutilizável após parsing
    ACPINVS = 4,               // ACPI NVS – não volátil, deve ser preservada
    BadMemory = 5,             // Áreas defeituosas; não devem ser usadas
    BootloaderReclaimable = 6, // Pode ser reutilizada após o boot
    KernelAndModules = 7,      // Ocupada pelo kernel e módulos (não padrão, mas alguns bootloaders usam)
    Unknown = 0xFFFFFFFF       // Para tipos não reconhecidos
};

enum class TagType : FK::dword {
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

enum class HeaderTagType : FK::dword {
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

enum class ConsoleFlags : FK::dword {
    ConsoleRequired = 1,
    EGATextSupported = 2
};

enum class Architecture : FK::dword { I386 = 0,
    MIPS32 = 4 };

enum class LoadPreference : FK::dword { None = 0,
    Low = 1,
    High = 2 };

struct alignas(TAG_ALIGN) Tag {
    TagType type;
    FK::dword size;

    Tag const* next() const noexcept
    {
        return reinterpret_cast<Tag const*>(
            reinterpret_cast<LibC::uint8_t const*>(this) + ((size + 7) & ~7u));
    }
};

static_assert(sizeof(Tag) == 8, "Tag struct must be exactly 8 bytes");
static_assert(alignof(Tag) == 8, "Tag struct must be aligned to 8 bytes");

struct alignas(TAG_ALIGN) TagString {
    Tag tag;
    char string[0];

    [[nodiscard]]
    char const* get_string() const noexcept
    {
        return reinterpret_cast<char const*>(this + 1);
    }
};

static_assert(alignof(TagString) == 8, "TagString must be aligned to 8 bytes");

struct alignas(TAG_ALIGN) TagModule {
    Tag tag;
    FK::dword mod_start;
    FK::dword mod_end;
    char cmdline[0];

    [[nodiscard]]
    char const* get_cmdline() const noexcept
    {
        return reinterpret_cast<char const*>(this + 1);
    }
};

static_assert(alignof(TagModule) == 8, "TagModule must be aligned to 8 bytes");

struct alignas(TAG_ALIGN) Info {
    FK::dword total_size;
    FK::dword reserved;
    Tag tags[0];

    [[nodiscard]]
    Tag const* first_tag() const noexcept
    {
        return tags;
    }
};
static_assert(sizeof(Info) == 8, "Info struct must be exactly 8 bytes");
static_assert(alignof(Info) == 8, "Info struct must be aligned to 8 bytes");

struct alignas(TAG_ALIGN) TagMemoryMap : Tag {
    struct Entry {
        FK::qword base_addr;
        FK::qword length;
        FK::dword type;
        FK::dword reserved;
    };
    static_assert(sizeof(Entry) == 24,
        "TagMemoryMap::Entry must be exactly 24 bytes");
    static_assert(alignof(Entry) == 8,
        "TagMemoryMap::Entry must be aligned to 8 bytes");

    FK::dword entry_size;
    FK::dword entry_version;
    Entry entries[0];

    Entry const* begin() const noexcept { return entries; }

    Entry const* end() const noexcept
    {
        LibC::size_t entries_size = size - sizeof(TagMemoryMap);
        LibC::size_t count = entries_size / entry_size;
        return entries + count;
    }
};
static_assert(alignof(TagMemoryMap) == 8,
    "TagMemoryMap must be aligned to 8 bytes");

}; // namespace multiboot2
