#include <Kernel/Hardware/Acpi/topology_manager.h>
#include <Kernel/Hardware/Acpi/acpi.h>
#include <LibFK/Algorithms/log.h>

namespace fkernel::acpi {

TopologyManager& TopologyManager::the() {
    static TopologyManager instance;
    return instance;
}

void TopologyManager::initialize() {
    auto* srat = static_cast<SRATHeader*>(ACPIManager::the().find_table("SRAT"));
    if (srat) {
        parse_srat(srat);
    } else {
        fk::algorithms::klog("TOPOLOGY", "SRAT table not found, system is UMA (Uniform Memory Access)");
    }
    m_is_initialized = true;
}

void TopologyManager::parse_srat(SRATHeader* srat) {
    fk::algorithms::klog("TOPOLOGY", "Parsing SRAT table (len=%u)", srat->header.length);
    
    uint8_t* ptr = reinterpret_cast<uint8_t*>(srat) + sizeof(SRATHeader);
    uint8_t* end = reinterpret_cast<uint8_t*>(srat) + srat->header.length;
    
    while (ptr < end) {
        auto* entry = reinterpret_cast<SRATEntryHeader*>(ptr);
        if (entry->length == 0) break;
        
        switch (entry->type) {
            case SRATEntryType::ProcessorLocalApic: {
                auto* lapic = reinterpret_cast<SRATProcessorLocalApic*>(ptr);
                if (lapic->flags & 1) { // Enabled
                    m_cpu_affinities.push_back({lapic->apic_id, lapic->proximity_domain()});
                    fk::algorithms::klog("TOPOLOGY", "  CPU APIC ID %u -> Proximity Domain %u", 
                                         lapic->apic_id, lapic->proximity_domain());
                }
                break;
            }
            case SRATEntryType::MemoryAffinity: {
                auto* mem = reinterpret_cast<SRATMemoryAffinity*>(ptr);
                if (mem->is_enabled()) {
                    m_nodes.push_back({mem->proximity_domain, mem->base_addr(), mem->length()});
                    fk::algorithms::klog("TOPOLOGY", "  Memory [0x%lx - 0x%lx] -> Proximity Domain %u", 
                                         mem->base_addr(), mem->base_addr() + mem->length(), mem->proximity_domain);
                }
                break;
            }
            case SRATEntryType::ProcessorLocalX2Apic: {
                auto* x2apic = reinterpret_cast<SRATProcessorLocalX2Apic*>(ptr);
                if (x2apic->flags & 1) {
                    m_cpu_affinities.push_back({x2apic->x2apic_id, x2apic->proximity_domain});
                    fk::algorithms::klog("TOPOLOGY", "  CPU x2APIC ID %u -> Proximity Domain %u", 
                                         x2apic->x2apic_id, x2apic->proximity_domain);
                }
                break;
            }
            default:
                break;
        }
        ptr += entry->length;
    }
}

uint32_t TopologyManager::get_node_for_paddr(uintptr_t phys) const {
    for (const auto& node : m_nodes) {
        if (phys >= node.base_addr && phys < node.base_addr + node.length) {
            return node.id;
        }
    }
    return 0; // Default to node 0 if not found
}

uint32_t TopologyManager::get_node_for_cpu(uint32_t apic_id) const {
    for (const auto& aff : m_cpu_affinities) {
        if (aff.apic_id == apic_id) {
            return aff.proximity_domain;
        }
    }
    return 0;
}

} // namespace fkernel::acpi
