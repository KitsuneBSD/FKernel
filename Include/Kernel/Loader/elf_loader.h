#pragma once

#include <Kernel/Loader/elf_types.h>
#include <LibFK/Core/Result.h>
#include <LibFK/Memory/ref_ptr.h>
#include <Kernel/Fs/Vfs/node.h>

namespace fkernel {

class ElfLoader {
public:
    static fk::core::Result<uintptr_t, fk::core::Error> load(fk::RefPtr<Node> node);

private:
    static fk::core::Result<uintptr_t, fk::core::Error> load_with_base(fk::RefPtr<Node> node, uintptr_t load_base);
};

} // namespace fkernel
