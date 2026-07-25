#pragma once

#include <Kernel/Loader/Types/elf64_types.h>
#include <Kernel/Loader/Types/elf_constants.h>

namespace fkernel {
struct Elf64_Phdr {
    Elf64_Word    p_type;
    Elf64_Word    p_flags;
    Elf64_Off     p_offset;
    Elf64_Addr    p_vaddr;
    Elf64_Addr    p_paddr;
    Elf64_Xword   p_filesz;
    Elf64_Xword   p_memsz;
    Elf64_Xword   p_align;
};
} // namespace fkernel