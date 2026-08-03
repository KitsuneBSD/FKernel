#pragma once

#include <Kernel/Memory/iommu.h>
#include <LibFK/Container/Sequence/vector.h>

namespace fkernel::vtd {

/**
 * @brief Intel VT-d (Virtualization Technology for Directed I/O) implementation.
 */
class IntelIOMMU final : public IOMMU {
public:
    static IntelIOMMU& the();

    void initialize();

    virtual fk::core::Result<void, fk::core::Error> create_domain(DomainId id) override;
    virtual fk::core::Result<void, fk::core::Error> map_device(DomainId domain, const PciDevice& device) override;
    virtual fk::core::Result<void, fk::core::Error> set_translation(DomainId domain, uintptr_t iova, uintptr_t phys, size_t size) override;
    
    virtual const char* name() const override { return "Intel VT-d"; }
    virtual bool is_active() const override { return m_active; }

private:
    IntelIOMMU() = default;

    struct RegisterMap {
        uint32_t version;
        uint32_t reserved0;
        uint64_t capability;
        uint64_t extended_capability;
        uint32_t global_command;
        uint32_t global_status;
        uint64_t root_entry_table;
        // ... many more
    } __attribute__((packed));

    bool m_active{false};
    uintptr_t m_re_table_phys{0};
};

} // namespace fkernel::vtd
