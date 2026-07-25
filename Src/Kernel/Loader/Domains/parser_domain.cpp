#include <Kernel/Loader/Domains/parser_domain.h>
#include <Kernel/Arch/x86_64/Interrupt/HardwareInterrupts/tick_manager.h>
#include <LibFK/Algorithms/log.h>

namespace fkernel::elf_domains {

ParserDomain::ParserDomain(fk::RefPtr<Node> node) : ElfDomain(node) {}

ElfHeaderResult ParserDomain::validate_header() {
  Elf64_Ehdr header;
  auto read_res = read_from_node(0, sizeof(Elf64_Ehdr), reinterpret_cast<uint8_t*>(&header));
  if (read_res.is_error())
    return read_res.error();

  if (read_res.value() < sizeof(Elf64_Ehdr)) {
    fk::algorithms::kwarn("ELF", "validate_header: Truncated read (%zu < %zu)", read_res.value(),
                          sizeof(Elf64_Ehdr));
    return fk::core::Error::InvalidParameter;
  }

  fk::algorithms::kdebug("ELF", "validate_header: Magic=%02x %c %c %c, Type=%u", header.e_ident[0],
                         header.e_ident[1], header.e_ident[2], header.e_ident[3],
                         (uint32_t)header.e_type);

  if (header.e_ident[0] != 0x7f || header.e_ident[1] != 'E' || header.e_ident[2] != 'L' ||
      header.e_ident[3] != 'F')
    return fk::core::Error::InvalidParameter;
  if (header.e_ident[4] != 2) {
    fk::algorithms::kwarn("ELF", "validate_header: Not a 64-bit ELF");
    return fk::core::Error::InvalidParameter;
  }
  if (header.e_machine != EM_X86_64) {
    fk::algorithms::kwarn("ELF", "validate_header: e_machine=%u, expected %u (x86_64)",
                          (unsigned)header.e_machine, (unsigned)EM_X86_64);
    return fk::core::Error::InvalidParameter;
  }
  if (header.e_phnum > ELF_MAX_PHNUM) {
    fk::algorithms::kwarn("ELF", "validate_header: e_phnum=%u exceeds limit %u",
                          (unsigned)header.e_phnum, (unsigned)ELF_MAX_PHNUM);
    return fk::core::Error::InvalidParameter;
  }
  if (header.e_phnum > 0 && header.e_phoff < sizeof(Elf64_Ehdr)) {
    fk::algorithms::kwarn("ELF", "validate_header: e_phoff=%llu overlaps ELF header",
                          (unsigned long long)header.e_phoff);
    return fk::core::Error::InvalidParameter;
  }

  return header;
}

ProgramHeadersResult ParserDomain::parse_program_headers(const Elf64_Ehdr& header) {
  fk::containers::Vector<Elf64_Phdr> headers;

  for (int i = 0; i < header.e_phnum; ++i) {
    Elf64_Phdr phdr;
    auto read_res = read_from_node(header.e_phoff + (i * header.e_phentsize), sizeof(Elf64_Phdr),
                                   reinterpret_cast<uint8_t*>(&phdr));
    if (read_res.is_error())
      return read_res.error();
    if (read_res.value() < sizeof(Elf64_Phdr))
      return fk::core::Error::InvalidParameter;

    headers.push_back(phdr);
  }

  return headers;
}

uint16_t ParserDomain::identify_executable_type(const Elf64_Ehdr& header) {
  return header.e_type;
}

static uintptr_t aslr_random_base() {
  uint64_t seed = TickManager::the().get_ticks();
  seed ^= seed >> 17;
  seed *= 0xBF58476D1CE4E5B9ULL;
  seed ^= seed >> 31;
  // Map to [0x10000000, 0x70000000) page-aligned
  static constexpr uintptr_t ASLR_MIN  = 0x10000000;
  static constexpr uintptr_t ASLR_RANGE = 0x60000000;
  return ASLR_MIN + ((seed & 0x0FFFF000ULL) % ASLR_RANGE & ~0xFFFULL);
}

uintptr_t ParserDomain::calculate_load_base(const Elf64_Ehdr& header, uintptr_t preferred_base) {
  if (header.e_type == ET_EXEC)
    return 0;
  if (header.e_type == ET_DYN && preferred_base == 0)
    return aslr_random_base();
  return preferred_base;
}

} // namespace fkernel::elf_domains