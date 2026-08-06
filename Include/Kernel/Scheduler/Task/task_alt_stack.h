#pragma once

#include <LibFK/Types/types.h>

struct TaskAltStack {
  void* ss_sp{nullptr};
  size_t ss_size{0};
  int ss_flags{0};
};
