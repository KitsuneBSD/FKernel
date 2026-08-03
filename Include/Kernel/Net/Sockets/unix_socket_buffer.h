#pragma once

#include <LibFK/Types/types.h>

namespace fkernel {

class UnixSocketBuffer {
public:
  static constexpr size_t CAPACITY = 4096;

  UnixSocketBuffer();
  ~UnixSocketBuffer();

  size_t read(uint8_t* buf, size_t max);
  size_t write(const uint8_t* src, size_t count);
  size_t available() const;
  size_t free_space() const;

private:
  uint8_t* m_data{nullptr};
  size_t   m_read_ptr{0};
  size_t   m_write_ptr{0};
};

} // namespace fkernel
