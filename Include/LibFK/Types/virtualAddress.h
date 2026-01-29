#pragma once

#include <LibFK/Types/types.h>

namespace fk {

class VirtualAddress {
public:
    constexpr VirtualAddress() : m_address(0) {}
    constexpr explicit VirtualAddress(uintptr_t address) : m_address(address) {}

    uintptr_t as_uintptr() const { return m_address; }
    void* as_ptr() const { return reinterpret_cast<void*>(m_address); }

    bool is_null() const { return m_address == 0; }
    bool is_page_aligned() const { return (m_address & 0xFFF) == 0; }

    VirtualAddress offset(int64_t bytes) const { return VirtualAddress(m_address + bytes); }

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
