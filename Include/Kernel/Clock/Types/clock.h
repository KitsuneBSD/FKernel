#pragma once

#include <LibFK/Core/Result.h>
#include <LibFK/Text/string.h>
#include <LibFK/Types/types.h>

#include <Kernel/Clock/Types/Datetime.h>

class Clock {
public:
  virtual ~Clock() = default;

  virtual fk::core::Result<void> initialize(uint32_t frequency) = 0;
  virtual fk::text::String get_name() = 0;
  virtual DateTime datetime() { return {}; }
};
