#include <Kernel/Boot/Uefi/elf_loader.h>
#include <Kernel/Boot/Uefi/fat.h>
#include <Kernel/Boot/Uefi/uefi_types.h>
#include <LibFK/Core/Assertions.h>
#include <LibC/string.h>

namespace uefi {

// ELF magic number
static constexpr unsigned char ELF_MAGIC[] = {0x7F, 'E', 'L', 'F'};

bool load_kernel(const fat::FatContext &fat_context, const char *filename, uint64_t &entry_point) {
  // Read ELF header
  Elf64_Ehdr elf_header;
  size_t bytes_read = 0;
  
  if (!fat::read_file(fat_context, filename, &elf_header, sizeof(Elf64_Ehdr), bytes_read)) {
    return false;
  }

  // Verify ELF magic
  if (memcmp(elf_header.e_ident, ELF_MAGIC, 4) != 0) {
    return false;
  }

  // Verify it's ELF64
  if (elf_header.e_ident[4] != 2) { // 1 = 32-bit, 2 = 64-bit
    return false;
  }

  // Verify it's x86_64
  if (elf_header.e_machine != 0x3E) { // 0x3E = x86_64
    return false;
  }

  entry_point = elf_header.e_entry;

  // Read program headers
  uint32_t phnum = elf_header.e_phnum;
  [[maybe_unused]] uint64_t phoff = elf_header.e_phoff;
  uint16_t phentsize = elf_header.e_phentsize;

  // Allocate buffer for program headers
  size_t ph_size = phnum * phentsize;
  uint8_t *ph_buffer = new uint8_t[ph_size];
  
  // Read program headers from file
  // For simplicity, we'll read them directly (assuming they're in the file)
  // In a real implementation, you'd need to handle the file offset properly
  size_t ph_bytes_read = 0;
  if (!fat::read_file(fat_context, filename, ph_buffer, ph_size, ph_bytes_read)) {
    delete[] ph_buffer;
    return false;
  }

  // Load each program segment
  for (uint32_t i = 0; i < phnum; ++i) {
    Elf64_Phdr *phdr = reinterpret_cast<Elf64_Phdr *>(ph_buffer + (i * phentsize));
    
    // Only load PT_LOAD segments (type 1)
    if (phdr->p_type != 1) {
      continue;
    }

    // Calculate segment size
    uint64_t mem_size = phdr->p_memsz;
    uint64_t file_size = phdr->p_filesz;
    uint64_t vaddr = phdr->p_vaddr;
    [[maybe_unused]] uint64_t offset = phdr->p_offset;

    // Allocate memory for segment
    void *segment_mem = reinterpret_cast<void *>(vaddr);
    
    // Read segment data from file
    // Note: This is simplified - in reality, you'd need to read from the correct file offset
    // and handle alignment properly
    uint8_t *segment_buffer = new uint8_t[file_size];
    size_t segment_bytes_read = 0;
    
    // For now, we'll assume the segments are loaded sequentially
    // A proper implementation would seek to the correct file offset
    if (!fat::read_file(fat_context, filename, segment_buffer, file_size, segment_bytes_read)) {
      delete[] segment_buffer;
      delete[] ph_buffer;
      return false;
    }

    // Copy segment to destination
    memcpy(segment_mem, segment_buffer, file_size);
    
    // Zero out BSS section if mem_size > file_size
    if (mem_size > file_size) {
      memset(reinterpret_cast<void *>(vaddr + file_size), 0, mem_size - file_size);
    }

    delete[] segment_buffer;
  }

  delete[] ph_buffer;
  return true;
}

} // namespace uefi
