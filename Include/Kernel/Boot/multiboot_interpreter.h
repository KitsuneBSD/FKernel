#pragma once

#include <Boot/multiboot2.h>
#include <LibFK/Log.h>

namespace multiboot2 {
class MultibootParser {
private:
  const uint8_t *base;

public:
  explicit MultibootParser(const void *mb_addr)
      : base(reinterpret_cast<const uint8_t *>(mb_addr)) {}

  const Tag *first_tag() const {
    return reinterpret_cast<const Tag *>(base +
                                         8); // Skip total_size + reserved
  }

  template <typename T> const T *find_tag(TagType type) const {
    for (const Tag *tag = first_tag(); tag && tag->type != TagType::End;
         tag = tag->next()) {
      if (tag->type == type) {
        Log(LogLevel::TRACE, "Found requested tag");
        return reinterpret_cast<const T *>(tag);
      }
    }
    Log(LogLevel::WARN, "Requested tag not found");
    return nullptr;
  }

  void foreach_tag(void (*callback)(const Tag *tag)) const {
    for (const Tag *tag = first_tag(); tag && tag->type != TagType::End;
         tag = tag->next()) {
      callback(tag);
    }
  }
};
}; // namespace multiboot2
