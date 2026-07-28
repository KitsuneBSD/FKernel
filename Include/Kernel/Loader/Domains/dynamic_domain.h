#pragma once

#include <Kernel/Loader/Domains/Base/elf_domain.h>
#include <Kernel/Loader/Types/elf64_dynamic.h>
#include <Kernel/Loader/Types/elf64_phdr.h>
#include <LibFK/Container/vector.h>
#include <LibFK/Core/result.h>
#include <LibFK/Text/string.h>
#include <LibFK/Types/virtual_address.h>

namespace fkernel::elf_domains {

struct LibraryContext {
    fk::VirtualAddress load_base;
    fk::VirtualAddress symtab;
    fk::VirtualAddress strtab;
    fk::text::String name;
};

class DynamicDomain : public ElfDomain {
public:
  explicit DynamicDomain(fk::RefPtr<Node> node) : ElfDomain(node) {}

  fk::core::Result<void, fk::core::Error>
  process_dynamic_segment(const fk::containers::Vector<Elf64_Phdr>& headers,
                          fk::VirtualAddress load_base);

  fk::core::Result<void, fk::core::Error>
  load_dependencies(const fk::containers::Vector<Elf64_Phdr>& headers,
                    fk::VirtualAddress load_base);

  fk::core::Result<void, fk::core::Error>
  apply_relocations(const fk::containers::Vector<Elf64_Phdr>& headers,
                    fk::VirtualAddress load_base);

  static fk::core::Result<LibraryContext, fk::core::Error>
  load_shared_library(const fk::text::String& name);

  const fk::containers::Vector<LibraryContext>& libraries() const { return m_libraries; }

private:
  struct RelaTable {
    fk::VirtualAddress addr;
    size_t             size{0};
    size_t             ent{sizeof(Elf64_Rela)};
  };

  struct SymbolContext {
    fk::VirtualAddress symtab;
    fk::VirtualAddress strtab;
  };

  fk::containers::Vector<LibraryContext> m_libraries;

  const Elf64_Phdr* find_dynamic_phdr(const fk::containers::Vector<Elf64_Phdr>& headers);
  RelaTable         extract_rela(const Elf64_Dyn* dyn, fk::VirtualAddress load_base);
  RelaTable         extract_jmprel(const Elf64_Dyn* dyn, fk::VirtualAddress load_base);
  SymbolContext     extract_symbols(const Elf64_Dyn* dyn, fk::VirtualAddress load_base);

  fk::core::Result<void, fk::core::Error>
  apply_rela_table(const RelaTable& table, const SymbolContext& ctx, fk::VirtualAddress load_base);

  fk::core::Result<void, fk::core::Error>
  apply_single_rela(const Elf64_Rela& rela, const SymbolContext& ctx, fk::VirtualAddress load_base);

  fk::VirtualAddress resolve_symbol(const SymbolContext& ctx, uint32_t sym_idx, fk::VirtualAddress load_base);
  fk::VirtualAddress resolve_symbol_cross(const SymbolContext& ctx, uint32_t sym_idx, fk::VirtualAddress load_base);
  const char*        symbol_name(const SymbolContext& ctx, uint32_t name_off);
};

} // namespace fkernel::elf_domains
