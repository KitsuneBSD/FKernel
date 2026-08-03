#pragma once

#include <Kernel/Loader/Types/elf64_dynamic.h>
#include <LibFK/Types/types.h>
#include <LibFK/Types/Memory/virtual_address.h>

namespace fkernel::elf_domains {

struct RelaTable {
  fk::VirtualAddress addr;
  size_t             size{0};
  size_t             ent{sizeof(Elf64_Rela)};
};

} // namespace fkernel::elf_domains
