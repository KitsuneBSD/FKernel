#pragma once

namespace fkernel {

class Timer {
public:
  virtual void initialize(uint32_t frequency) = 0;
  virtual ~Timer() = default;
};

} // namespace fkernel
