#pragma once

#include <LibFK/Text/string.h>
#include <LibFK/Types/Memory/virtual_address.h>

namespace fkernel::elf_domains {

struct LibraryContext {
  fk::VirtualAddress load_base;
  fk::VirtualAddress symtab;
  fk::VirtualAddress strtab;
  fk::text::String name;
};

} // namespace fkernel::elf_domains
