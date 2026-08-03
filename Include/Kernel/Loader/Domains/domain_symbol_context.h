#pragma once

#include <LibFK/Types/Memory/virtual_address.h>

namespace fkernel::elf_domains {

struct SymbolContext {
  fk::VirtualAddress symtab;
  fk::VirtualAddress strtab;
};

} // namespace fkernel::elf_domains
