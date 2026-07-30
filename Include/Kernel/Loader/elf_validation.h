#pragma once

#include <Kernel/Loader/Types/elf64_ehdr.h>
#include <Kernel/Loader/Types/elf_constants.h>
#include <LibFK/Core/result.h>

namespace fkernel {

// Pure validation of an already-read Elf64_Ehdr — no I/O, no hardware.
// Returns the same header on success so callers can use TRY() pattern.
// Checks: ELF magic, little-endian, 64-bit class, x86_64 machine,
//         e_phnum within ELF_MAX_PHNUM, e_phoff beyond ELF header.
inline fk::core::Result<Elf64_Ehdr> elf_check_header(const Elf64_Ehdr& hdr) {
    if (hdr.e_ident[0] != 0x7f || hdr.e_ident[1] != 'E' ||
        hdr.e_ident[2] != 'L'  || hdr.e_ident[3] != 'F')
        return fk::core::Error::InvalidParameter;

    if (hdr.e_ident[EI_DATA] != ELFDATA2LSB)
        return fk::core::Error::InvalidParameter;

    // e_ident[4] == ELFCLASS64 == 2
    if (hdr.e_ident[4] != 2)
        return fk::core::Error::InvalidParameter;

    if (hdr.e_machine != EM_X86_64)
        return fk::core::Error::InvalidParameter;

    if (hdr.e_phnum > ELF_MAX_PHNUM)
        return fk::core::Error::InvalidParameter;

    if (hdr.e_phnum > 0 && hdr.e_phoff < sizeof(Elf64_Ehdr))
        return fk::core::Error::InvalidParameter;

    return hdr;
}

} // namespace fkernel
