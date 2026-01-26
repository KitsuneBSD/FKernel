#pragma once

#include <LibFK/Types/types.h>
#include <LibFK/Core/Result.h>
#include <LibFK/Memory/ref_ptr.h>
#include <Kernel/Fs/Vfs/node.h>

namespace fkernel {

typedef uint64_t Elf64_Addr;
typedef uint64_t Elf64_Off;
typedef uint16_t Elf64_Half;
typedef uint32_t Elf64_Word;
typedef int32_t  Elf64_Sword;
typedef uint64_t Elf64_Xword;
typedef int64_t  Elf64_Sxword;

#define EI_NIDENT 16

struct Elf64_Ehdr {
    unsigned char e_ident[EI_NIDENT];
    Elf64_Half    e_type;
    Elf64_Half    e_machine;
    Elf64_Word    e_version;
    Elf64_Addr    e_entry;
    Elf64_Off     e_phoff;
    Elf64_Off     e_shoff;
    Elf64_Word    e_flags;
    Elf64_Half    e_ehsize;
    Elf64_Half    e_phentsize;
    Elf64_Half    e_phnum;
    Elf64_Half    e_shentsize;
    Elf64_Half    e_shnum;
    Elf64_Half    e_shstrndx;
};

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

#define PT_LOAD 1

#define PF_X 1
#define PF_W 2
#define PF_R 4

class ElfLoader {
public:
    static fk::core::Result<uintptr_t, fk::core::Error> load(fk::RefPtr<Node> node);

private:
    static fk::core::Result<uintptr_t, fk::core::Error> load_with_base(fk::RefPtr<Node> node, uintptr_t load_base);
    static bool validate_header(const Elf64_Ehdr& header);
};

} // namespace fkernel
