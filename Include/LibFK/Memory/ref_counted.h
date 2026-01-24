#pragma once

#include <LibFK/Types/types.h>
#include <LibFK/Memory/new.h> // For operator delete

namespace fk {
namespace memory {

template <typename T>
class RefCounted {
public:
    void ref() const {
        m_ref_count++;
    }

    void unref() const {
        if (--m_ref_count == 0) {
            delete static_cast<const T*>(this);
        }
    }

    uint32_t ref_count() const { return m_ref_count; }

protected:
    RefCounted() = default;
    virtual ~RefCounted() = default;

private:
    mutable uint32_t m_ref_count { 1 };
};

} // namespace memory
} // namespace fk
