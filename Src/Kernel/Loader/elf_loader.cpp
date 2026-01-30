#include <Kernel/Loader/elf_loader.h>
#include <Kernel/Loader/elf_loader_core.h>

namespace fkernel {

ElfLoadOperationResult
ElfLoader::load(fk::RefPtr<Node> node) {
    return load_with_base(node, 0);
}

ElfLoadOperationResult
ElfLoader::load_with_base(fk::RefPtr<Node> node, uintptr_t load_base) {
    ElfLoaderCore core(node);
    return core.execute_load_with_base(load_base);
}

} // namespace fkernel
