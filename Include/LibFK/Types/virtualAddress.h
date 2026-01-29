#pragma once

#include <LibFK/Types/types.h>
#include <LibFK/Core/Assertions.h>

namespace fk {

class VirtualAddress {
public:
    constexpr VirtualAddress() : m_address(0) {}
    constexpr explicit VirtualAddress(uintptr_t address) : m_address(address) {
        if (address != 0) {
            // Check for canonical address form (bits 48-63 must match bit 47)
            bool is_canonical = (((int64_t)address >> 47) == 0) || (((int64_t)address >> 47) == -1);
            ASSERT(is_canonical && "Attempted to create a non-canonical VirtualAddress");
        }
    }

    uintptr_t as_uintptr() const { return m_address; }
    void* as_ptr() const { return reinterpret_cast<void*>(m_address); }

    explicit operator bool() const { return m_address != 0; }
    bool is_null() const { return m_address == 0; }
    bool is_page_aligned() const { return (m_address & 0xFFF) == 0; }

    bool is_canonical() const {
        return ((int64_t)m_address >> 47) == 0 || ((int64_t)m_address >> 47) == -1;
    }

    VirtualAddress offset(int64_t bytes) const { 
        VirtualAddress new_addr(m_address + bytes);
        return new_addr;
    }

    bool operator==(const VirtualAddress& other) const { return m_address == other.m_address; }
    bool operator!=(const VirtualAddress& other) const { return m_address != other.m_address; }
    bool operator<(const VirtualAddress& other) const { return m_address < other.m_address; }
    bool operator>(const VirtualAddress& other) const { return m_address > other.m_address; }
    bool operator<=(const VirtualAddress& other) const { return m_address <= other.m_address; }
    bool operator>=(const VirtualAddress& other) const { return m_address >= other.m_address; }

private:
    uintptr_t m_address;
};

} // namespace fk
