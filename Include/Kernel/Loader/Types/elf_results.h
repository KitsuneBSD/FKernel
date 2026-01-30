#pragma once

#include <Kernel/Loader/Types/elf64_types.h>
#include <Kernel/Loader/Types/elf_constants.h>
#include <Kernel/Loader/Types/elf64_ehdr.h>
#include <Kernel/Loader/Types/elf64_phdr.h>
#include <LibFK/Core/Result.h>
#include <LibFK/Container/vector.h>

namespace fkernel {
// Result types for ELF operations
using ElfHeaderResult = fk::core::Result<Elf64_Ehdr, fk::core::Error>;
using ProgramHeadersResult = fk::core::Result<fk::containers::Vector<Elf64_Phdr>, fk::core::Error>;

struct ElfLoadResult {
    uintptr_t entry;
    uintptr_t ph_addr;
    uint16_t ph_num;
    uint16_t ph_ent;
};

using ElfLoadOperationResult = fk::core::Result<ElfLoadResult, fk::core::Error>;

} // namespace fkernel