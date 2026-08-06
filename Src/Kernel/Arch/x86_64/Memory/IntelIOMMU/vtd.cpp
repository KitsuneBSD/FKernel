#include <LibFK/Algorithms/Logging/log.h>
#include <LibFK/Utilities/memory.h>

#include <Kernel/Arch/x86_64/Memory/IntelIOMMU/vtd.h>
#include <Kernel/Hardware/Firmware/Acpi/acpi.h>
#include <Kernel/Hardware/Firmware/Acpi/dmar.h>
#include <Kernel/Memory/memory_manager.h>
#include <Kernel/Memory/PhysicalMemory/physical_memory_manager.h>

namespace fkernel::vtd {

IntelIOMMU& IntelIOMMU::the() {
    static IntelIOMMU instance;
    return instance;
}

void IntelIOMMU::initialize() {
    auto* dmar = static_cast<acpi::DMARHeader*>(ACPIManager::the().find_table("DMAR"));
    if (!dmar) {
        fk::algorithms::klog("IOMMU", "DMAR table not found, Intel VT-d not supported.");
        return;
    }

    fk::algorithms::klog("IOMMU", "Found DMAR table at %p, Host Address Width: %u", 
                         dmar, dmar->host_address_width + 1);

    uint8_t* ptr = reinterpret_cast<uint8_t*>(dmar) + sizeof(acpi::DMARHeader);
    uint8_t* end = reinterpret_cast<uint8_t*>(dmar) + dmar->header.length;

    while (ptr < end) {
        auto* entry = reinterpret_cast<acpi::DMAREntryHeader*>(ptr);
        if (entry->length == 0) break;

        if (entry->type == acpi::DMARType::DRHD) {
            auto* drhd = reinterpret_cast<acpi::DMAR_DRHD*>(ptr);
            fk::algorithms::klog("IOMMU", "  Found DRHD at 0x%lx (Segment: %u, Flags: 0x%x)", 
                                 drhd->register_base_address, drhd->segment_number, drhd->flags);
            
            // Map IOMMU registers
            MemoryManager::the().map_page(drhd->register_base_address, drhd->register_base_address, 
                                         PageFlags::Present | PageFlags::Writable | PageFlags::CacheDisabled);
            
            // For now, we just track the first one
            if (m_re_table_phys == 0) {
                // Initialize Root Entry Table
                m_re_table_phys = PhysicalMemoryManager::the().alloc_page();
                fk::memory::set(reinterpret_cast<void*>(m_re_table_phys), 0, 4096);
                
                auto* regs = reinterpret_cast<volatile RegisterMap*>(drhd->register_base_address);
                regs->root_entry_table = m_re_table_phys;
                regs->global_command = (1u << 25); // Set Root Table Pointer (RTPS)
                
                fk::algorithms::klog("IOMMU", "  Root Entry Table initialized at %p", (void*)m_re_table_phys);
            }
        }
        ptr += entry->length;
    }

    m_active = true;
    fk::algorithms::klog("IOMMU", "Intel VT-d initialized successfully.");
}

fk::core::Result<void, fk::core::Error> IntelIOMMU::create_domain(DomainId id) {
    (void)id;
    return fk::core::Error::NotImplemented;
}

fk::core::Result<void, fk::core::Error> IntelIOMMU::map_device(DomainId domain, const PciDevice& device) {
    (void)domain; (void)device;
    return fk::core::Error::NotImplemented;
}

fk::core::Result<void, fk::core::Error> IntelIOMMU::set_translation(DomainId domain, uintptr_t iova, uintptr_t phys, size_t size) {
    (void)domain; (void)iova; (void)phys; (void)size;
    return fk::core::Error::NotImplemented;
}

} // namespace fkernel::vtd
