#pragma once

#include <Kernel/MemoryManagement/PhysicalMemoryManagement/PhysicalMemoryManager.h>
#include <Kernel/MemoryManagement/PhysicalMemoryManagement/PhysicalMemoryRegion.h>
#include <Kernel/MemoryManagement/VirtualMemoryManagement/VirtualMemoryManagement.h>

namespace MemoryManagement {
class MemoryManager {
private:
    bool is_initialized = false;

    MemoryManager() = default;

public:
    static MemoryManager& instance()
    {
        static MemoryManager instance;
        return instance;
    }

    void initialize(multiboot2::TagMemoryMap const& mmap);
};
}
