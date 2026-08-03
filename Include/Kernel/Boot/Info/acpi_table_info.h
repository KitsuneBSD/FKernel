#pragma once

#include <LibFK/Types/types.h>

namespace boot {

struct AcpiTableInfo {
  void *rsdp{nullptr};
  void *rsdt{nullptr};
  void *xsdt{nullptr};
};

} // namespace boot
