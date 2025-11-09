#pragma once

#include <Kernel/Boot/multiboot2.h>

namespace multiboot2 {
class MultibootParser {
private:
  uint8_t const *base;

public:
  explicit MultibootParser(void const *mb_addr)
      : base(reinterpret_cast<uint8_t const *>(mb_addr)) {}

  Tag const *first_tag() const {
    return reinterpret_cast<Tag const *>(base +
                                         8); // Skip total_size + reserved
  }

  template <typename T> T const *find_tag(TagType type) const {
    for (Tag const *tag = first_tag(); tag && tag->type != TagType::End;
         tag = tag->next()) {
      if (tag->type == type) {
        return reinterpret_cast<T const *>(tag);
      }
    }
    return nullptr;
  }

  void foreach_tag(void (*callback)(Tag const *tag)) const {
    for (Tag const *tag = first_tag(); tag && tag->type != TagType::End;
         tag = tag->next()) {
      callback(tag);
    }
  }
};

inline bool is_available(uint32_t type) {
  auto t = static_cast<MemoryType>(type);
  return t == MemoryType::Available || t == MemoryType::BootloaderReclaimable ||
         t == MemoryType::ACPIReclaimable;
}

}; // namespace multiboot2
