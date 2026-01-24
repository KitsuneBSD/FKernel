#include <Kernel/Loader/elf_loader.h>
#include <Kernel/Memory/PhysicalMemory/physical_memory_manager.h>
#include <Kernel/Memory/VirtualMemory/Pages/page_flags.h>
#include <Kernel/Memory/VirtualMemory/virtual_memory_manager.h>
#include <Kernel/Fs/Vfs/virtual_filesystem.h>
#include <LibFK/Utilities/Memory.h>
#include <LibFK/Algorithms/log.h>

namespace fkernel {

#define PT_INTERP 3

bool ElfLoader::validate_header(const Elf64_Ehdr &header) {
  if (header.e_ident[0] != 0x7f || header.e_ident[1] != 'E' ||
      header.e_ident[2] != 'L' || header.e_ident[3] != 'F')
    return false;
  if (header.e_ident[4] != 2)
    return false; // 64-bit
  return true;
}

fk::core::Result<uintptr_t, fk::core::Error>
ElfLoader::load(fk::RefPtr<Node> node) {
  Elf64_Ehdr header;
  auto read_res =
      node->read(0, sizeof(Elf64_Ehdr), reinterpret_cast<uint8_t *>(&header));
  if (read_res.is_error())
    return read_res.error();
  if (read_res.value() < sizeof(Elf64_Ehdr))
    return fk::core::Error::InvalidParameter;

  if (!validate_header(header)) {
    fk::algorithms::kerror("ELF", "Invalid ELF header");
    return fk::core::Error::InvalidParameter;
  }

  fk::algorithms::klog("ELF", "Header type: %u, Entry: %p, PHNum: %u", 
                       (uint32_t)header.e_type, (void*)header.e_entry, (uint32_t)header.e_phnum);

  uintptr_t load_base = 0;
  fk::text::String interpreter_path;

  // 1. Scan for PT_INTERP first
  for (int i = 0; i < header.e_phnum; ++i) {
    Elf64_Phdr phdr;
    node->read(header.e_phoff + (i * header.e_phentsize), sizeof(Elf64_Phdr),
               reinterpret_cast<uint8_t *>(&phdr));

    if (phdr.p_type == PT_INTERP) {
        char path_buf[256];
        size_t len = phdr.p_filesz < 255 ? phdr.p_filesz : 255;
        node->read(phdr.p_offset, len, reinterpret_cast<uint8_t*>(path_buf));
        path_buf[len] = '\0';
        interpreter_path = path_buf;
        fk::algorithms::klog("ELF", "Dynamic linker requested: %s", path_buf);
    }
  }

  // 2. Load Program Headers
  for (int i = 0; i < header.e_phnum; ++i) {
    Elf64_Phdr phdr;
    node->read(header.e_phoff + (i * header.e_phentsize), sizeof(Elf64_Phdr),
               reinterpret_cast<uint8_t *>(&phdr));

    if (phdr.p_type == PT_LOAD) {
      uintptr_t vaddr_with_base = phdr.p_vaddr + load_base;
      
      // Allocate and map pages
      uintptr_t start_vaddr = vaddr_with_base & ~0xFFFULL;
      uintptr_t end_vaddr =
          (vaddr_with_base + phdr.p_memsz + 0xFFF) & ~0xFFFULL;

      for (uintptr_t vaddr = start_vaddr; vaddr < end_vaddr; vaddr += 0x1000) {
        uintptr_t existing_phys = VirtualMemoryManager::the().translate(vaddr);

        if (existing_phys == 0 || existing_phys == vaddr) {
          uintptr_t phys = PhysicalMemoryManager::the().alloc_page();
          VirtualMemoryManager::the().map_page(
              vaddr, phys,
              PageFlags::Present | PageFlags::Writable | PageFlags::User);
          fk::memory::set(reinterpret_cast<void *>(vaddr), 0, 0x1000);
        }
      }

      // Copy data from segment
      node->read(phdr.p_offset, phdr.p_filesz,
                 reinterpret_cast<uint8_t *>(vaddr_with_base));
    }
  }

  // 3. If an interpreter is needed, load it and return its entry point
  if (!interpreter_path.is_empty()) {
      auto interp_node_res = VirtualFileSystem::the().resolve_path(interpreter_path.c_str());
      if (interp_node_res.is_error()) {
          fk::algorithms::kerror("ELF", "Could not find dynamic linker: %s", interpreter_path.c_str());
          return interp_node_res.error();
      }
      
      // NOTE: Interpreters (ld.so) are usually ET_DYN and need a non-zero load_base.
      // For now, we simplify and assume they load at a fixed high address or handle themselves.
      // Real implementation would call load recursively with a base offset.
      return load(interp_node_res.value());
  }

  return (uintptr_t)header.e_entry + load_base;
}

} // namespace fkernel
