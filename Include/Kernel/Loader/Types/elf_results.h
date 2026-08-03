#pragma once

#include <Kernel/Loader/Types/elf64_types.h>
#include <Kernel/Loader/Types/elf_constants.h>
#include <Kernel/Loader/Types/elf64_ehdr.h>
#include <Kernel/Loader/Types/elf64_phdr.h>
#include <LibFK/Core/result.h>
#include <LibFK/Container/Sequence/vector.h>

namespace fkernel {
// Result types for ELF operations
using ElfHeaderResult = fk::core::Result<Elf64_Ehdr, fk::core::Error>;
using ProgramHeadersResult = fk::core::Result<fk::containers::Vector<Elf64_Phdr>, fk::core::Error>;

struct TlsInfo {
    uintptr_t vaddr{0};
    size_t    filesz{0};
    size_t    memsz{0};
    size_t    align{8};
    bool      present{false};
};

struct ElfLoadResult {
    uintptr_t entry;
    uintptr_t ph_addr;
    uint16_t  ph_num;
    uint16_t  ph_ent;
    bool      stack_executable{false};
    TlsInfo   tls;
    uintptr_t highest_load_end{0};
    uintptr_t init{0};
    uintptr_t fini{0};
    uintptr_t init_array{0};
    uintptr_t fini_array{0};
    size_t    init_arraysz{0};
    size_t    fini_arraysz{0};
};

using ElfLoadOperationResult = fk::core::Result<ElfLoadResult, fk::core::Error>;

} // namespace fkernel