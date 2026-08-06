#pragma once
#include <LibFK/Types/types.h>
namespace fkernel::acpi {
enum class SRATEntryType : uint8_t {
    ProcessorLocalApic   = 0,
    MemoryAffinity       = 1,
    ProcessorLocalX2Apic = 2,
    GicItAffinity        = 3,
};
} // namespace fkernel::acpi
