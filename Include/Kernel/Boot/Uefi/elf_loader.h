#pragma once

#include <Kernel/Boot/Uefi/uefi_types.h>
#include <Kernel/Boot/Uefi/fat.h>
#include <LibFK/Types/types.h>

namespace uefi {

/**
 * @brief ELF64 header structure
 */
struct Elf64_Ehdr {
  unsigned char e_ident[16];
  uint16_t e_type;
  uint16_t e_machine;
  uint32_t e_version;
  uint64_t e_entry;
  uint64_t e_phoff;
  uint64_t e_shoff;
  uint32_t e_flags;
  uint16_t e_ehsize;
  uint16_t e_phentsize;
  uint16_t e_phnum;
  uint16_t e_shentsize;
  uint16_t e_shnum;
  uint16_t e_shstrndx;
};

/**
 * @brief ELF64 program header structure
 */
struct Elf64_Phdr {
  uint32_t p_type;
  uint32_t p_flags;
  uint64_t p_offset;
  uint64_t p_vaddr;
  uint64_t p_paddr;
  uint64_t p_filesz;
  uint64_t p_memsz;
  uint64_t p_align;
};

/**
 * @brief Load ELF64 kernel from FAT filesystem
 * @param fat_context FAT filesystem context
 * @param filename Kernel filename
 * @param entry_point Output: kernel entry point address
 * @return true on success, false on failure
 */
bool load_kernel(const fat::FatContext &fat_context,
                 const char *filename,
                 uint64_t &entry_point);

} // namespace uefi
