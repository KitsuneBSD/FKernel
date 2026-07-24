#pragma once

#include <Kernel/Loader/Domains/Base/elf_domain.h>
#include <Kernel/Loader/Types/elf64_dynamic.h>
#include <Kernel/Loader/Types/elf64_phdr.h>
#include <LibFK/Container/vector.h>
#include <LibFK/Core/result.h>

namespace fkernel::elf_domains {

class DynamicDomain : public ElfDomain {
public:
  explicit DynamicDomain(fk::RefPtr<Node> node) : ElfDomain(node) {}

  fk::core::Result<void, fk::core::Error>
  process_dynamic_segment(const fk::containers::Vector<Elf64_Phdr>& headers,
                          uintptr_t load_base);

private:
  struct RelaTable {
    uintptr_t addr{0};
    size_t    size{0};
    size_t    ent{sizeof(Elf64_Rela)};
  };

  struct SymbolContext {
    uintptr_t symtab{0};
    uintptr_t strtab{0};
  };

  const Elf64_Phdr* find_dynamic_phdr(const fk::containers::Vector<Elf64_Phdr>& headers);
  RelaTable         extract_rela(const Elf64_Dyn* dyn, uintptr_t load_base);
  RelaTable         extract_jmprel(const Elf64_Dyn* dyn, uintptr_t load_base);
  SymbolContext     extract_symbols(const Elf64_Dyn* dyn, uintptr_t load_base);

  fk::core::Result<void, fk::core::Error>
  apply_rela_table(const RelaTable& table, const SymbolContext& ctx, uintptr_t load_base);

  fk::core::Result<void, fk::core::Error>
  apply_single_rela(const Elf64_Rela& rela, const SymbolContext& ctx, uintptr_t load_base);

  uintptr_t    resolve_symbol(const SymbolContext& ctx, uint32_t sym_idx, uintptr_t load_base);
  const char*  symbol_name(const SymbolContext& ctx, uint32_t name_off);
};

} // namespace fkernel::elf_domains
