#pragma once

#include <Kernel/Loader/Types/elf64_types.h>

namespace fkernel {

struct Elf64_Dyn {
  Elf64_Sxword d_tag;
  union {
    Elf64_Xword d_val;
    Elf64_Addr  d_ptr;
  } d_un;
};

struct Elf64_Sym {
  Elf64_Word  st_name;
  uint8_t     st_info;
  uint8_t     st_other;
  Elf64_Half  st_shndx;
  Elf64_Addr  st_value;
  Elf64_Xword st_size;
};

struct Elf64_Rela {
  Elf64_Addr   r_offset;
  Elf64_Xword  r_info;
  Elf64_Sxword r_addend;
};

// Dynamic tag values
#define DT_NULL         0
#define DT_NEEDED       1
#define DT_PLTRELSZ     2
#define DT_PLTGOT       3
#define DT_HASH         4
#define DT_STRTAB       5
#define DT_SYMTAB       6
#define DT_RELA         7
#define DT_RELASZ       8
#define DT_RELAENT      9
#define DT_STRSZ       10
#define DT_SYMENT      11
#define DT_INIT        12
#define DT_FINI        13
#define DT_SONAME      14
#define DT_RPATH       15
#define DT_SYMBOLIC    16
#define DT_REL         17
#define DT_RELSZ       18
#define DT_RELENT      19
#define DT_PLTREL      20
#define DT_DEBUG       21
#define DT_TEXTREL     22
#define DT_JMPREL      23
#define DT_BIND_NOW    24
#define DT_INIT_ARRAY  25
#define DT_FINI_ARRAY  26
#define DT_INIT_ARRAYSZ 27
#define DT_FINI_ARRAYSZ 28
#define DT_RUNPATH     29
#define DT_FLAGS       30
#define DT_ENCODING    32
#define DT_GNU_HASH    0x6ffffef5
#define DT_VERSYM      0x6ffffff0
#define DT_VERNEED     0x6ffffffe
#define DT_VERNEEDNUM  0x6fffffff

#define DF_ORIGIN  0x1
#define DF_SYMBOLIC 0x2
#define DF_TEXTREL 0x4
#define DF_BIND_NOW 0x8
#define DF_STATIC_TLS 0x10

// x86-64 relocation types
#define R_X86_64_NONE      0
#define R_X86_64_64        1
#define R_X86_64_COPY      5
#define R_X86_64_GLOB_DAT  6
#define R_X86_64_JUMP_SLOT 7
#define R_X86_64_RELATIVE  8
#define R_X86_64_IRELATIVE 37
#define R_X86_64_TPOFF64   18
#define R_X86_64_DTPMOD64  16
#define R_X86_64_DTPOFF64  17

// Special section indices
#define SHN_UNDEF  0
#define SHN_ABS    0xFFF1
#define SHN_COMMON 0xFFF2

// Symbol binding
#define STB_LOCAL  0
#define STB_GLOBAL 1
#define STB_WEAK   2

// Relocation info field accessors
#define ELF64_R_SYM(i)   (static_cast<uint32_t>((i) >> 32))
#define ELF64_R_TYPE(i)  (static_cast<uint32_t>((i) & 0xFFFFFFFFULL))

// Symbol info field accessor
#define ELF64_ST_BIND(i) ((i) >> 4)
#define ELF64_ST_TYPE(i) ((i) & 0xF)

} // namespace fkernel
