#pragma once

#include <Kernel/Boot/multiboot2.h>

namespace multiboot2 {
class MultibootParser {
private:
    LibC::uint8_t const* base;

public:
    explicit MultibootParser(void const* mb_addr)
        : base(reinterpret_cast<LibC::uint8_t const*>(mb_addr))
    {
    }

    Tag const* first_tag() const
    {
        return reinterpret_cast<Tag const*>(base + 8); // Skip total_size + reserved
    }

    template<typename T>
    T const* find_tag(TagType type) const
    {
        for (Tag const* tag = first_tag(); tag && tag->type != TagType::End;
             tag = tag->next()) {
            if (tag->type == type) {
                return reinterpret_cast<T const*>(tag);
            }
        }
        return nullptr;
    }

    void foreach_tag(void (*callback)(Tag const* tag)) const
    {
        for (Tag const* tag = first_tag(); tag && tag->type != TagType::End;
             tag = tag->next()) {
            callback(tag);
        }
    }
};
}; // namespace multiboot2
