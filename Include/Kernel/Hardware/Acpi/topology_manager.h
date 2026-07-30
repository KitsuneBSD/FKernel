#pragma once

#include <Kernel/Hardware/Acpi/srat.h>
#include <LibFK/Container/vector.h>
#include <LibFK/Core/result.h>

namespace fkernel::acpi {

struct NUMANode {
    uint32_t id;
    uint64_t base_addr;
    uint64_t length;
};

struct CpuAffinity {
    uint32_t apic_id;
    uint32_t proximity_domain;
};

class TopologyManager {
public:
    static TopologyManager& the();

    bool is_initialized() const { return m_is_initialized; }
    void initialize();

    const fk::containers::Vector<NUMANode>& nodes() const { return m_nodes; }
    const fk::containers::Vector<CpuAffinity>& cpu_affinities() const { return m_cpu_affinities; }

    uint32_t get_node_for_paddr(uintptr_t phys) const;
    uint32_t get_node_for_cpu(uint32_t apic_id) const;

private:
    TopologyManager() = default;
    TopologyManager(const TopologyManager&) = delete;
    TopologyManager& operator=(const TopologyManager&) = delete;

    bool m_is_initialized{false};
    fk::containers::Vector<NUMANode> m_nodes;
    fk::containers::Vector<CpuAffinity> m_cpu_affinities;

    void parse_srat(SRATHeader* srat);
};

} // namespace fkernel::acpi
