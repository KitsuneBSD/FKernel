#include <LibFK/Algorithms/Crypto/chacha20.h>
#include <LibFK/Algorithms/Logging/log.h>
#include <Kernel/Loader/Domains/parser_domain.h>
#include <Kernel/Loader/elf_validation.h>

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

  auto check = fkernel::elf_check_header(header);
  if (check.is_error()) {
    fk::algorithms::kwarn("ELF", "validate_header: header check failed");
    return check.error();
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
  uint64_t seed = 0;
  fk::algorithms::ChaCha20PRNG::the().fill_buffer(reinterpret_cast<uint8_t*>(&seed), sizeof(seed));
  static constexpr uintptr_t ASLR_MIN   = 0x10000000;
  static constexpr uintptr_t ASLR_PAGES = 0x60000000 / 0x1000;
  return ASLR_MIN + ((seed % ASLR_PAGES) * 0x1000);
}

uintptr_t ParserDomain::calculate_load_base(const Elf64_Ehdr& header, uintptr_t preferred_base) {
  if (header.e_type == ET_EXEC)
    return 0;
  if (header.e_type == ET_DYN && preferred_base == 0)
    return aslr_random_base();
  return preferred_base;
}

} // namespace fkernel::elf_domains