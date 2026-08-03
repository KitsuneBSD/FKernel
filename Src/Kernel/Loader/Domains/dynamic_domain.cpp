#include <Kernel/Loader/Domains/dynamic_domain.h>
#include <Kernel/Loader/Domains/load_domain.h>
#include <Kernel/Loader/Domains/memory_domain.h>
#include <Kernel/Loader/Domains/parser_domain.h>
#include <Kernel/Fs/Vfs/Core/virtual_filesystem.h>
#include <Kernel/Arch/x86_64/Hardware/Cpu/cpu_ops.h>
#include <LibFK/Algorithms/Logging/log.h>
#include <LibFK/Synchronization/spinlock.h>
#include <LibFK/Utilities/memory.h>

namespace fkernel::elf_domains {

static fk::containers::Vector<LibraryContext> s_global_libraries;
static fk::synchronization::Spinlock s_library_lock;

fk::core::Result<void, fk::core::Error>
DynamicDomain::process_dynamic_segment(const fk::containers::Vector<Elf64_Phdr>& headers,
                                       fk::VirtualAddress load_base) {
  const Elf64_Phdr* phdr = find_dynamic_phdr(headers);
  if (!phdr)
    return {};

  auto dep_res = load_dependencies(headers, load_base);
  if (dep_res.is_error())
    return dep_res;

  for (auto& lib : m_libraries)
    DynamicDomain::load_shared_library(lib.name);

  return apply_relocations(headers, load_base);
}

fk::core::Result<void, fk::core::Error>
DynamicDomain::load_dependencies(const fk::containers::Vector<Elf64_Phdr>& headers,
                                  fk::VirtualAddress load_base) {
  const Elf64_Phdr* phdr = find_dynamic_phdr(headers);
  if (!phdr)
    return {};

  uintptr_t dyn_vaddr = phdr->p_vaddr + load_base.as_uintptr();
  const auto* dyn = reinterpret_cast<const Elf64_Dyn*>(dyn_vaddr);

  fk::VirtualAddress strtab{};
  for (const Elf64_Dyn* d = dyn; d->d_tag != DT_NULL; ++d) {
    if (d->d_tag == DT_STRTAB) { strtab = fk::VirtualAddress(d->d_un.d_ptr + load_base.as_uintptr()); break; }
  }

  for (const Elf64_Dyn* d = dyn; d->d_tag != DT_NULL; ++d) {
    if (d->d_tag != DT_NEEDED) continue;
    if (!strtab) continue;

    const char* name = reinterpret_cast<const char*>(strtab.as_uintptr() + d->d_un.d_val);
    if (!name || name[0] == '\0') continue;

  bool already_loaded = false;
  {
    fk::synchronization::ScopedLockIRQ lock(s_library_lock);
    for (auto& s : s_global_libraries) {
      if (s.name == name) { already_loaded = true; break; }
    }
    if (!already_loaded)
      s_global_libraries.push_back({fk::VirtualAddress(0), fk::VirtualAddress(0), fk::VirtualAddress(0), fk::text::String(name)});
  }
  }

  return {};
}

fk::core::Result<LibraryContext, fk::core::Error>
DynamicDomain::load_shared_library(const fk::text::String& name) {
  {
    fk::synchronization::ScopedLockIRQ lock(s_library_lock);
    for (auto& s : s_global_libraries) {
      if (s.name == name) {
        if (s.load_base) return s;
        break;
      }
    }
  }

  fk::text::String path_str = "/lib/";
  path_str += name;
  const char* libname = path_str.c_str();

  auto dentry_res = VirtualFileSystem::the().resolve_path(libname);
  if (dentry_res.is_error()) {
    fk::algorithms::kwarn("ELF", "Shared library not found: %s (tried %s)", name.c_str(), libname);
    return dentry_res.error();
  }
  auto node = dentry_res.value()->top_node();

  ParserDomain parser(node);
  auto header_res = parser.validate_header();
  if (header_res.is_error()) return header_res.error();

  auto headers_res = parser.parse_program_headers(header_res.value());
  if (headers_res.is_error()) return headers_res.error();

  uintptr_t base = parser.calculate_load_base(header_res.value(), 0);

  LoadDomain loader(node);
  auto load_res = loader.process_load_segments(headers_res.value(), base);
  if (load_res.is_error()) return load_res.error();

  SymbolContext ctx;
  {
    const Elf64_Phdr* dyn_phdr = nullptr;
    for (const auto& h : headers_res.value()) {
      if (h.p_type == PT_DYNAMIC) { dyn_phdr = &h; break; }
    }
    if (dyn_phdr) {
      uintptr_t dv = dyn_phdr->p_vaddr + base;
      const auto* dyn_arr = reinterpret_cast<const Elf64_Dyn*>(dv);
      DynamicDomain lib_dd(node);
      for (const Elf64_Dyn* d = dyn_arr; d->d_tag != DT_NULL; ++d) {
        if (d->d_tag == DT_SYMTAB) ctx.symtab = fk::VirtualAddress(d->d_un.d_ptr + base);
        if (d->d_tag == DT_STRTAB) ctx.strtab = fk::VirtualAddress(d->d_un.d_ptr + base);
      }
      lib_dd.apply_relocations(headers_res.value(), fk::VirtualAddress(base));
    }
  }

  LibraryContext result;
  result.load_base = fk::VirtualAddress(base);
  result.symtab    = ctx.symtab;
  result.strtab    = ctx.strtab;
  result.name      = name;

  {
    fk::synchronization::ScopedLockIRQ lock(s_library_lock);
    for (auto& s : s_global_libraries) {
      if (s.name == name) {
        s.load_base = fk::VirtualAddress(base);
        s.symtab    = ctx.symtab;
        s.strtab    = ctx.strtab;
        break;
      }
    }
  }

  fk::algorithms::klog("ELF", "Loaded shared library: %s at 0x%lx", libname, base);
  return result;
}

fk::core::Result<void, fk::core::Error>
DynamicDomain::apply_relocations(const fk::containers::Vector<Elf64_Phdr>& headers,
                                  fk::VirtualAddress load_base) {
  const Elf64_Phdr* phdr = find_dynamic_phdr(headers);
  if (!phdr)
    return {};

  uintptr_t dyn_vaddr = phdr->p_vaddr + load_base.as_uintptr();
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

RelaTable
DynamicDomain::extract_rela(const Elf64_Dyn* dyn, fk::VirtualAddress load_base) {
  RelaTable table;
  for (const Elf64_Dyn* d = dyn; d->d_tag != DT_NULL; ++d) {
    if (d->d_tag == DT_RELA)   table.addr = fk::VirtualAddress(d->d_un.d_ptr + load_base.as_uintptr());
    if (d->d_tag == DT_RELASZ) table.size = static_cast<size_t>(d->d_un.d_val);
    if (d->d_tag == DT_RELAENT) table.ent = static_cast<size_t>(d->d_un.d_val);
  }
  return table;
}

RelaTable
DynamicDomain::extract_jmprel(const Elf64_Dyn* dyn, fk::VirtualAddress load_base) {
  RelaTable table;
  table.ent = sizeof(Elf64_Rela);
  for (const Elf64_Dyn* d = dyn; d->d_tag != DT_NULL; ++d) {
    if (d->d_tag == DT_JMPREL)   table.addr = fk::VirtualAddress(d->d_un.d_ptr + load_base.as_uintptr());
    if (d->d_tag == DT_PLTRELSZ) table.size = static_cast<size_t>(d->d_un.d_val);
  }
  return table;
}

SymbolContext
DynamicDomain::extract_symbols(const Elf64_Dyn* dyn, fk::VirtualAddress load_base) {
  SymbolContext ctx;
  for (const Elf64_Dyn* d = dyn; d->d_tag != DT_NULL; ++d) {
    if (d->d_tag == DT_SYMTAB) ctx.symtab = fk::VirtualAddress(d->d_un.d_ptr + load_base.as_uintptr());
    if (d->d_tag == DT_STRTAB) ctx.strtab = fk::VirtualAddress(d->d_un.d_ptr + load_base.as_uintptr());
  }
  return ctx;
}

fk::core::Result<void, fk::core::Error>
DynamicDomain::apply_rela_table(const RelaTable& table, const SymbolContext& ctx,
                                fk::VirtualAddress load_base) {
  size_t ent = table.ent ? table.ent : sizeof(Elf64_Rela);
  size_t count = table.size / ent;

  for (size_t i = 0; i < count; ++i) {
    const auto* rela =
        reinterpret_cast<const Elf64_Rela*>(table.addr.as_uintptr() + i * ent);
    auto res = apply_single_rela(*rela, ctx, load_base);
    if (res.is_error())
      return res;
  }

  return {};
}

fk::core::Result<void, fk::core::Error>
DynamicDomain::apply_single_rela(const Elf64_Rela& rela, const SymbolContext& ctx,
                                 fk::VirtualAddress load_base) {
  uint32_t type    = ELF64_R_TYPE(rela.r_info);
  uint32_t sym_idx = ELF64_R_SYM(rela.r_info);
  auto* target     = reinterpret_cast<uintptr_t*>(rela.r_offset + load_base.as_uintptr());

  switch (type) {
    case R_X86_64_NONE:
      return {};
    case R_X86_64_RELATIVE:
      arch_smap_begin();
      *target = load_base.as_uintptr() + static_cast<uintptr_t>(rela.r_addend);
      arch_smap_end();
      return {};
    case R_X86_64_64: {
      fk::VirtualAddress sym_val = resolve_symbol_cross(ctx, sym_idx, load_base);
      arch_smap_begin();
      *target = sym_val.as_uintptr() + static_cast<uintptr_t>(rela.r_addend);
      arch_smap_end();
      return {};
    }
    case R_X86_64_GLOB_DAT:
    case R_X86_64_JUMP_SLOT: {
      fk::VirtualAddress sym_val = resolve_symbol_cross(ctx, sym_idx, load_base);
      arch_smap_begin();
      *target = sym_val.as_uintptr() + static_cast<uintptr_t>(rela.r_addend);
      arch_smap_end();
      return {};
    }
    case R_X86_64_COPY: {
      fk::VirtualAddress sym_val = resolve_symbol_cross(ctx, sym_idx, load_base);
      if (sym_val && rela.r_addend > 0) {
        arch_smap_begin();
        fk::memory::copy(reinterpret_cast<void*>(rela.r_offset + load_base.as_uintptr()),
                         reinterpret_cast<const void*>(sym_val.as_uintptr()),
                         static_cast<size_t>(rela.r_addend));
        arch_smap_end();
      }
      return {};
    }
    case R_X86_64_IRELATIVE:
      if (rela.r_addend) {
        using ifunc_t = uintptr_t (*)();
        auto ifunc = reinterpret_cast<ifunc_t>(load_base.as_uintptr() + static_cast<uintptr_t>(rela.r_addend));
        arch_smap_begin();
        *target = ifunc();
        arch_smap_end();
      }
      return {};
    case R_X86_64_TPOFF64: {
      fk::VirtualAddress sym_val = resolve_symbol_cross(ctx, sym_idx, load_base);
      arch_smap_begin();
      *target = sym_val.as_uintptr() + static_cast<uintptr_t>(rela.r_addend);
      arch_smap_end();
      return {};
    }
    case R_X86_64_DTPMOD64:
      arch_smap_begin();
      *target = 1;
      arch_smap_end();
      return {};
    case R_X86_64_DTPOFF64: {
      fk::VirtualAddress sym_val = resolve_symbol_cross(ctx, sym_idx, load_base);
      arch_smap_begin();
      *target = sym_val.as_uintptr() + static_cast<uintptr_t>(rela.r_addend);
      arch_smap_end();
      return {};
    }
    default:
      fk::algorithms::kdebug("ELF", "Unsupported relocation type %u at 0x%lx",
                             type, (unsigned long)rela.r_offset);
      return {};
  }
}

fk::VirtualAddress
DynamicDomain::resolve_symbol(const SymbolContext& ctx, uint32_t sym_idx, fk::VirtualAddress load_base) {
  if (!ctx.symtab || sym_idx == 0)
    return fk::VirtualAddress(0);

  const auto* sym = reinterpret_cast<const Elf64_Sym*>(ctx.symtab.as_uintptr()) + sym_idx;
  if (sym->st_shndx == SHN_UNDEF) {
    const char* name = symbol_name(ctx, sym->st_name);
    fk::algorithms::kdebug("ELF", "Unresolved symbol: %s", name ? name : "<unknown>");
    return fk::VirtualAddress(0);
  }
  if (sym->st_shndx == SHN_ABS)
    return fk::VirtualAddress(static_cast<uintptr_t>(sym->st_value));
  if (sym->st_shndx == SHN_COMMON) {
    fk::algorithms::kdebug("ELF", "COMMON symbol: %s", symbol_name(ctx, sym->st_name));
    return fk::VirtualAddress(0);
  }
  return fk::VirtualAddress(static_cast<uintptr_t>(sym->st_value) + load_base.as_uintptr());
}

fk::VirtualAddress
DynamicDomain::resolve_symbol_cross(const SymbolContext& ctx, uint32_t sym_idx, fk::VirtualAddress load_base) {
  auto result = resolve_symbol(ctx, sym_idx, load_base);
  if (result) return result;

  if (sym_idx == 0) return fk::VirtualAddress(0);

  const auto* sym = reinterpret_cast<const Elf64_Sym*>(ctx.symtab.as_uintptr()) + sym_idx;
  if (sym->st_shndx != SHN_UNDEF) {
    if (sym->st_shndx == SHN_COMMON) {
      const char* name = symbol_name(ctx, sym->st_name);
      fk::algorithms::kdebug("ELF", "COMMON symbol not allocated: %s", name ? name : "<unknown>");
    }
    return fk::VirtualAddress(0);
  }

  const char* name = symbol_name(ctx, sym->st_name);
  if (!name || name[0] == '\0') return fk::VirtualAddress(0);

  {
    fk::synchronization::ScopedLockIRQ lock(s_library_lock);
    for (auto& lib : s_global_libraries) {
    if (!lib.symtab || !lib.strtab) continue;
    SymbolContext lib_ctx{lib.symtab, lib.strtab};
    auto val = resolve_symbol(lib_ctx, sym_idx, lib.load_base);
    if (val) return val;

    const auto* lib_symtab = reinterpret_cast<const Elf64_Sym*>(lib.symtab.as_uintptr());
    for (uint32_t i = 1; i < 65536; ++i) {
      const auto* lib_sym = lib_symtab + i;
      if (lib_sym->st_shndx == SHN_UNDEF) continue;
      const char* lib_name = reinterpret_cast<const char*>(lib.strtab.as_uintptr() + lib_sym->st_name);
      if (lib_name && fk::memory::compare(name, lib_name, fk::memory::length(name) + 1) == 0 &&
          lib_name[fk::memory::length(name)] == '\0') {
        return fk::VirtualAddress(static_cast<uintptr_t>(lib_sym->st_value) + lib.load_base.as_uintptr());
      }
    }
    }
  }

  fk::algorithms::kdebug("ELF", "Unresolved symbol (cross-object): %s", name);
  return fk::VirtualAddress(0);
}

const char*
DynamicDomain::symbol_name(const SymbolContext& ctx, uint32_t name_off) {
  if (!ctx.strtab)
    return nullptr;
  return reinterpret_cast<const char*>(ctx.strtab.as_uintptr() + name_off);
}

} // namespace fkernel::elf_domains
