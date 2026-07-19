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
#define DT_NULL     0
#define DT_NEEDED   1
#define DT_PLTRELSZ 2
#define DT_PLTGOT   3
#define DT_STRTAB   5
#define DT_SYMTAB   6
#define DT_RELA     7
#define DT_RELASZ   8
#define DT_RELAENT  9
#define DT_STRSZ    10
#define DT_PLTREL   20
#define DT_JMPREL   23

// x86-64 relocation types
#define R_X86_64_NONE      0
#define R_X86_64_64        1
#define R_X86_64_RELATIVE  8
#define R_X86_64_GLOB_DAT  6
#define R_X86_64_JUMP_SLOT 7

// Special section indices
#define SHN_UNDEF 0
#define SHN_ABS   0xFFF1

// Relocation info field accessors
#define ELF64_R_SYM(i)   (static_cast<uint32_t>((i) >> 32))
#define ELF64_R_TYPE(i)  (static_cast<uint32_t>((i) & 0xFFFFFFFFULL))

// Symbol info field accessor
#define ELF64_ST_BIND(i) ((i) >> 4)
#define ELF64_ST_TYPE(i) ((i) & 0xF)

} // namespace fkernel
