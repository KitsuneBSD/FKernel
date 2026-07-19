#include <Kernel/Loader/Domains/dynamic_domain.h>
#include <LibFK/Algorithms/log.h>

namespace fkernel::elf_domains {

fk::core::Result<void, fk::core::Error>
DynamicDomain::process_dynamic_segment(const fk::containers::Vector<Elf64_Phdr>& headers,
                                       uintptr_t load_base) {
  const Elf64_Phdr* phdr = find_dynamic_phdr(headers);
  if (!phdr)
    return {};

  uintptr_t dyn_vaddr = phdr->p_vaddr + load_base;
  const auto* dyn = reinterpret_cast<const Elf64_Dyn*>(dyn_vaddr);

  auto ctx    = extract_symbols(dyn, load_base);
  auto rela   = extract_rela(dyn, load_base);
  auto jmprel = extract_jmprel(dyn, load_base);

  if (rela.addr) {
    auto res = apply_rela_table(rela, ctx, load_base);
    if (res.is_error())
      return res;
  }
  if (jmprel.addr) {
    auto res = apply_rela_table(jmprel, ctx, load_base);
    if (res.is_error())
      return res;
  }

  return {};
}

const Elf64_Phdr*
DynamicDomain::find_dynamic_phdr(const fk::containers::Vector<Elf64_Phdr>& headers) {
  for (const auto& phdr : headers) {
    if (phdr.p_type == PT_DYNAMIC)
      return &phdr;
  }
  return nullptr;
}

DynamicDomain::RelaTable
DynamicDomain::extract_rela(const Elf64_Dyn* dyn, uintptr_t load_base) {
  RelaTable table;
  for (const Elf64_Dyn* d = dyn; d->d_tag != DT_NULL; ++d) {
    if (d->d_tag == DT_RELA)   table.addr = d->d_un.d_ptr + load_base;
    if (d->d_tag == DT_RELASZ) table.size = static_cast<size_t>(d->d_un.d_val);
    if (d->d_tag == DT_RELAENT) table.ent = static_cast<size_t>(d->d_un.d_val);
  }
  return table;
}

DynamicDomain::RelaTable
DynamicDomain::extract_jmprel(const Elf64_Dyn* dyn, uintptr_t load_base) {
  RelaTable table;
  table.ent = sizeof(Elf64_Rela);
  for (const Elf64_Dyn* d = dyn; d->d_tag != DT_NULL; ++d) {
    if (d->d_tag == DT_JMPREL)   table.addr = d->d_un.d_ptr + load_base;
    if (d->d_tag == DT_PLTRELSZ) table.size = static_cast<size_t>(d->d_un.d_val);
  }
  return table;
}

DynamicDomain::SymbolContext
DynamicDomain::extract_symbols(const Elf64_Dyn* dyn, uintptr_t load_base) {
  SymbolContext ctx;
  for (const Elf64_Dyn* d = dyn; d->d_tag != DT_NULL; ++d) {
    if (d->d_tag == DT_SYMTAB) ctx.symtab = d->d_un.d_ptr + load_base;
    if (d->d_tag == DT_STRTAB) ctx.strtab = d->d_un.d_ptr + load_base;
  }
  return ctx;
}

fk::core::Result<void, fk::core::Error>
DynamicDomain::apply_rela_table(const RelaTable& table, const SymbolContext& ctx,
                                uintptr_t load_base) {
  size_t ent = table.ent ? table.ent : sizeof(Elf64_Rela);
  size_t count = table.size / ent;

  for (size_t i = 0; i < count; ++i) {
    const auto* rela =
        reinterpret_cast<const Elf64_Rela*>(table.addr + i * ent);
    auto res = apply_single_rela(*rela, ctx, load_base);
    if (res.is_error())
      return res;
  }

  return {};
}

fk::core::Result<void, fk::core::Error>
DynamicDomain::apply_single_rela(const Elf64_Rela& rela, const SymbolContext& ctx,
                                 uintptr_t load_base) {
  uint32_t type    = ELF64_R_TYPE(rela.r_info);
  uint32_t sym_idx = ELF64_R_SYM(rela.r_info);
  auto* target     = reinterpret_cast<uintptr_t*>(rela.r_offset + load_base);

  switch (type) {
    case R_X86_64_NONE:
      return {};
    case R_X86_64_RELATIVE:
      *target = load_base + static_cast<uintptr_t>(rela.r_addend);
      return {};
    case R_X86_64_64: {
      uintptr_t sym_val = resolve_symbol(ctx, sym_idx, load_base);
      *target = sym_val + static_cast<uintptr_t>(rela.r_addend);
      return {};
    }
    case R_X86_64_GLOB_DAT:
    case R_X86_64_JUMP_SLOT:
      *target = resolve_symbol(ctx, sym_idx, load_base);
      return {};
    default:
      fk::algorithms::kwarn("ELF", "Unsupported relocation type %u at 0x%lx",
                            type, (unsigned long)rela.r_offset);
      return {};
  }
}

uintptr_t
DynamicDomain::resolve_symbol(const SymbolContext& ctx, uint32_t sym_idx, uintptr_t load_base) {
  if (!ctx.symtab || sym_idx == 0)
    return 0;

  const auto* sym = reinterpret_cast<const Elf64_Sym*>(ctx.symtab) + sym_idx;
  if (sym->st_shndx == SHN_UNDEF) {
    const char* name = symbol_name(ctx, sym->st_name);
    fk::algorithms::kwarn("ELF", "Unresolved symbol: %s", name ? name : "<unknown>");
    return 0;
  }
  if (sym->st_shndx == SHN_ABS)
    return static_cast<uintptr_t>(sym->st_value);
  return static_cast<uintptr_t>(sym->st_value) + load_base;
}

const char*
DynamicDomain::symbol_name(const SymbolContext& ctx, uint32_t name_off) {
  if (!ctx.strtab)
    return nullptr;
  return reinterpret_cast<const char*>(ctx.strtab + name_off);
}

} // namespace fkernel::elf_domains
