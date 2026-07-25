#pragma once

#include <Kernel/Fs/Vfs/node.h>
#include <LibFK/Core/result.h>
#include <LibFK/Memory/ref_ptr.h>

namespace fkernel::elf_domains {

class ElfDomain {
public:
    virtual ~ElfDomain() = default;
    
protected:
    fk::RefPtr<Node> m_node;
    
    explicit ElfDomain(fk::RefPtr<Node> node) : m_node(node) {}
    
    fk::core::Result<size_t, fk::core::Error> read_from_node(uint64_t offset, size_t size, uint8_t* buffer) {
        return m_node->read(offset, size, buffer);
    }
};

} // namespace fkernel::elf_domains