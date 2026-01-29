#pragma once

#include <LibFK/Types/types.h>
#include <LibFK/Core/Assertions.h>

namespace fk {

class PhysicalAddress {
public:
    constexpr PhysicalAddress() : m_address(0) {}
    constexpr explicit PhysicalAddress(uintptr_t address) : m_address(address) {
        // x86_64 physical addresses are limited to 52 bits
        ASSERT((address >> 52) == 0);
    }

    uintptr_t as_uintptr() const { return m_address; }

    bool is_null() const { return m_address == 0; }
    bool is_page_aligned() const { return (m_address & 0xFFF) == 0; }

    PhysicalAddress offset(int64_t bytes) const { return PhysicalAddress(m_address + bytes); }

    bool operator==(const PhysicalAddress& other) const { return m_address == other.m_address; }
    bool operator!=(const PhysicalAddress& other) const { return m_address != other.m_address; }
    bool operator<(const PhysicalAddress& other) const { return m_address < other.m_address; }
    bool operator>(const PhysicalAddress& other) const { return m_address > other.m_address; }

private:
    uintptr_t m_address;
};

} // namespace fk
