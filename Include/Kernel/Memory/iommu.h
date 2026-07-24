#pragma once

#include <LibFK/Core/result.h>
#include <LibFK/Core/error.h>
#include <LibFK/Types/types.h>
#include <Kernel/Hardware/Pci/pci_device.h>

namespace fkernel {

using DomainId = uint32_t;

/**
 * @class IOMMU
 * @brief Abstract base class for Input/Output Memory Management Unit support.
 * 
 * Provides DMA protection and address translation per PCI device.
 */
class IOMMU {
public:
    virtual ~IOMMU() = default;

    virtual fk::core::Result<void, fk::core::Error> create_domain(DomainId id) = 0;
    virtual fk::core::Result<void, fk::core::Error> map_device(DomainId domain, const PciDevice& device) = 0;
    virtual fk::core::Result<void, fk::core::Error> set_translation(DomainId domain, uintptr_t iova, uintptr_t phys, size_t size) = 0;
    
    virtual const char* name() const = 0;
    virtual bool is_active() const = 0;

protected:
    IOMMU() = default;
};

} // namespace fkernel
