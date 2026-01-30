#pragma once

#include <Kernel/Loader/elf_types.h>
#include <LibFK/Core/Result.h>
#include <LibFK/Memory/ref_ptr.h>
#include <Kernel/Fs/Vfs/node.h>

namespace fkernel {

class ElfLoader {
public:
    static ElfLoadOperationResult load(fk::RefPtr<Node> node);

private:
    static ElfLoadOperationResult load_with_base(fk::RefPtr<Node> node, uintptr_t load_base);
};

} // namespace fkernel
