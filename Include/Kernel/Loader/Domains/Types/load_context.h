#pragma once

#include <Kernel/Loader/Types/elf64_ehdr.h>
#include <LibFK/Text/string.h>

namespace fkernel::elf_domains {
struct LoadContext {
    uintptr_t load_base;
    Elf64_Ehdr header;
    fk::text::String interpreter_path;
    bool has_interpreter;
    uintptr_t interpreter_entry;
};
} // namespace fkernel::elf_domains